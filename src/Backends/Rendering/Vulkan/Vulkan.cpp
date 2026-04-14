// Vulkan rendering backend
// Implements all gl*() functions declared in Backends/Rendering.h using Vulkan.
// Emulates OpenGL 1.x fixed-function pipeline via SPIR-V shaders and CPU-side state tracking.

#include "Backends/Rendering.h"
#include "Backends/Shared/Vulkan.h"
#include "Vulkan_Shaders.h"

#include <vulkan/vulkan.h>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <algorithm>

// ============================================================================
// 4x4 Matrix Math (column-major, matching OpenGL convention)
// ============================================================================

namespace mat4
{

static void identity(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

static void copy(float* dst, const float* src)
{
    memcpy(dst, src, 64);
}

// C = A * B (column-major)
static void multiply(float* C, const float* A, const float* B)
{
    float tmp[16];
    for (int c = 0; c < 4; c++)
    {
        for (int r = 0; r < 4; r++)
        {
            tmp[c * 4 + r] =
                A[0 * 4 + r] * B[c * 4 + 0] +
                A[1 * 4 + r] * B[c * 4 + 1] +
                A[2 * 4 + r] * B[c * 4 + 2] +
                A[3 * 4 + r] * B[c * 4 + 3];
        }
    }
    memcpy(C, tmp, 64);
}

static void translate(float* M, float x, float y, float z)
{
    float T[16];
    identity(T);
    T[12] = x; T[13] = y; T[14] = z;
    float tmp[16];
    multiply(tmp, M, T);
    copy(M, tmp);
}

static void rotate(float* M, float angle_deg, float ax, float ay, float az)
{
    float rad = angle_deg * 3.14159265358979323846f / 180.0f;
    float c = cosf(rad), s = sinf(rad);
    float len = sqrtf(ax * ax + ay * ay + az * az);
    if (len < 1e-8f) return;
    ax /= len; ay /= len; az /= len;
    float nc = 1.0f - c;

    float R[16];
    R[0] = ax * ax * nc + c;       R[4] = ax * ay * nc - az * s;   R[8]  = ax * az * nc + ay * s;  R[12] = 0;
    R[1] = ay * ax * nc + az * s;  R[5] = ay * ay * nc + c;        R[9]  = ay * az * nc - ax * s;  R[13] = 0;
    R[2] = az * ax * nc - ay * s;  R[6] = az * ay * nc + ax * s;   R[10] = az * az * nc + c;       R[14] = 0;
    R[3] = 0;                       R[7] = 0;                        R[11] = 0;                       R[15] = 1;

    float tmp[16];
    multiply(tmp, M, R);
    copy(M, tmp);
}

static void scale(float* M, float x, float y, float z)
{
    float S[16];
    identity(S);
    S[0] = x; S[5] = y; S[10] = z;
    float tmp[16];
    multiply(tmp, M, S);
    copy(M, tmp);
}

// Vulkan ortho: z maps to [0,1], Y negated to flip from GL's Y-up to Vulkan's Y-down
static void ortho(float* M, double l, double r, double b, double t, double n, double f)
{
    float O[16];
    memset(O, 0, 64);
    O[0]  = (float)(2.0 / (r - l));
    O[5]  = (float)(-2.0 / (t - b));              // negated Y
    O[10] = (float)(-1.0 / (f - n));
    O[12] = (float)(-(r + l) / (r - l));
    O[13] = (float)((t + b) / (t - b));            // negated Y
    O[14] = (float)(-n / (f - n));
    O[15] = 1.0f;

    float tmp[16];
    multiply(tmp, M, O);
    copy(M, tmp);
}

// Vulkan frustum: z maps to [0,1], Y negated to flip from GL's Y-up to Vulkan's Y-down
static void frustum(float* M, double l, double r, double b, double t, double n, double f)
{
    float F[16];
    memset(F, 0, 64);
    F[0]  = (float)(2.0 * n / (r - l));
    F[5]  = (float)(-2.0 * n / (t - b));           // negated Y
    F[8]  = (float)((r + l) / (r - l));
    F[9]  = (float)(-(t + b) / (t - b));            // negated Y
    F[10] = (float)(f / (n - f));
    F[11] = -1.0f;
    F[14] = (float)(n * f / (n - f));

    float tmp[16];
    multiply(tmp, M, F);
    copy(M, tmp);
}

// Transform a 4-component vector by a matrix: out = M * v
static void transformVec4(float* out, const float* M, const float* v)
{
    for (int r = 0; r < 4; r++)
    {
        out[r] = M[0 * 4 + r] * v[0] +
                 M[1 * 4 + r] * v[1] +
                 M[2 * 4 + r] * v[2] +
                 M[3 * 4 + r] * v[3];
    }
}

} // namespace mat4

// ============================================================================
// Vulkan Backend State
// ============================================================================

namespace
{

// Matrix stacks
struct MatrixStack
{
    float matrices[32][16];
    int top = 0;

    MatrixStack()
    {
        mat4::identity(matrices[0]);
    }

    float* current() { return matrices[top]; }
    const float* current() const { return matrices[top]; }

    void push()
    {
        if (top < 31)
        {
            mat4::copy(matrices[top + 1], matrices[top]);
            top++;
        }
    }

    void pop()
    {
        if (top > 0)
            top--;
    }

    void loadIdentity()
    {
        mat4::identity(current());
    }
};

MatrixStack mvStack, projStack, texStack;
int currentMatrixMode = GL_MODELVIEW;

MatrixStack& getActiveStack()
{
    switch (currentMatrixMode)
    {
        case GL_PROJECTION: return projStack;
        case GL_TEXTURE: return texStack;
        default: return mvStack;
    }
}

// Render state (identical to D3D11)
struct
{
    float color[4] = {1, 1, 1, 1};
    float normal[3] = {0, 0, 1};

    bool texture2D = false;
    bool blend = false;
    bool alphaTest = false;
    bool depthTest = false;
    bool depthWrite = true;
    bool cullFace = false;
    bool fog = false;
    bool lighting = false;
    bool light0 = false;
    bool light1 = false;
    bool colorMaterial = false;
    bool polyOffsetFill = false;
    bool logicOp = false;
    bool rescaleNormal = false;
    bool normalizeEnabled = false;

    GLenum blendSrc = GL_ONE;
    GLenum blendDst = GL_ZERO;

    GLenum depthFunc = GL_LESS;
    double clearDepth = 1.0;

    GLenum alphaFunc = GL_ALWAYS;
    float alphaRef = 0.0f;

    GLenum fogMode = GL_EXP;
    float fogDensity = 1.0f;
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    float fogColor[4] = {0, 0, 0, 0};

    float lightDir[2][4] = {{0, 0, 1, 0}, {0, 0, 1, 0}};
    float lightDiffuse[2][4] = {{1, 1, 1, 1}, {0, 0, 0, 1}};
    float lightAmbient[2][4] = {{0, 0, 0, 1}, {0, 0, 0, 1}};
    float globalAmbient[4] = {0.2f, 0.2f, 0.2f, 1.0f};

    bool colorMask[4] = {true, true, true, true};

    float polyOffsetFactor = 0.0f;
    float polyOffsetUnits = 0.0f;

    GLenum logicOpMode = GL_COPY;

    float clearColor[4] = {0, 0, 0, 0};

    int viewport[4] = {0, 0, 854, 480};

    GLuint boundTexture = 0;

    bool vertexArray = false;
    bool texCoordArray = false;
    bool colorArray = false;
    bool normalArray = false;

    struct VertexPointer { int size; GLenum type; int stride; const void* ptr; };
    VertexPointer vp = {3, GL_FLOAT, 0, nullptr};
    VertexPointer tp = {2, GL_FLOAT, 0, nullptr};
    VertexPointer cp = {4, GL_UNSIGNED_BYTE, 0, nullptr};
    struct NormalPointer { GLenum type; int stride; const void* ptr; };
    NormalPointer np = {GL_BYTE, 0, nullptr};

    GLuint boundVBO = 0;

    GLenum shadeModel = GL_SMOOTH;
} state;

// Constant buffer data (384 bytes, std140 compatible — matches shader UBO)
struct alignas(16) CBData
{
    float mvp[16];
    float mv[16];
    float texMat[16];

    float lightDir0[4];
    float lightDir1[4];
    float lightDiffuse0[4];
    float lightDiffuse1[4];
    float globalAmbient[4];

    float currentColor[4];
    float currentNormal[4];

    uint32_t lightingEnabled;
    uint32_t textureEnabled;
    uint32_t hasVertexColor;
    uint32_t hasVertexNormal;
    uint32_t hasVertexTexCoord;

    uint32_t _pad1a;
    uint32_t _pad1b;
    uint32_t _pad1c;

    float fogColor[4];
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint32_t fogMode;

    float alphaRef;
    uint32_t alphaTestEnabled;

    float _pad2a;
    float _pad2b;
};
static_assert(sizeof(CBData) % 16 == 0, "CBData must be 16-byte aligned");

// ============================================================================
// Vulkan Resources
// ============================================================================

static constexpr int MAX_FRAMES = 2;
static constexpr uint32_t MAX_DESCRIPTOR_SETS = 16384;
static constexpr size_t DYNAMIC_VB_SIZE = 32 * 1024 * 1024; // 32MB per frame
static constexpr size_t UBO_SIZE = MAX_DESCRIPTOR_SETS * 512; // ~8MB per frame (512 bytes aligned per draw)

VkShaderModule g_vertShaderModule = VK_NULL_HANDLE;
VkShaderModule g_fragShaderModule = VK_NULL_HANDLE;
VkPipelineLayout g_pipelineLayout = VK_NULL_HANDLE;
VkDescriptorSetLayout g_descriptorSetLayout = VK_NULL_HANDLE;

// Deferred buffer deletion (wait until GPU is done with the frame)
struct DeferredBuffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

// Per-frame resources
struct FrameResources
{
    VkBuffer uboBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uboMemory = VK_NULL_HANDLE;
    void* uboMapped = nullptr;
    uint32_t uboOffset = 0;

    VkBuffer dynamicVB = VK_NULL_HANDLE;
    VkDeviceMemory dynamicVBMemory = VK_NULL_HANDLE;
    void* dynamicVBMapped = nullptr;
    size_t dynamicVBOffset = 0;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    uint32_t descriptorSetIndex = 0;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<DeferredBuffer> pendingDeletes;
};

FrameResources g_frames[MAX_FRAMES];

// Pipeline cache
std::unordered_map<uint64_t, VkPipeline>& g_pipelineCache()
{
    static std::unordered_map<uint64_t, VkPipeline> inst;
    return inst;
}

// Sampler cache
std::unordered_map<uint32_t, VkSampler>& g_samplerCache()
{
    static std::unordered_map<uint32_t, VkSampler> inst;
    return inst;
}

// UBO alignment
uint32_t g_uboAlignment = 256;

// Track which frame we last reset for
int g_lastResetFrame = -1;

bool g_initialized = false;

// Per-frame state tracking to avoid redundant Vulkan API calls
VkImageView     g_lastBoundImageView  = VK_NULL_HANDLE;
VkSampler       g_lastBoundSampler    = VK_NULL_HANDLE;
VkDescriptorSet g_lastDescriptorSet   = VK_NULL_HANDLE;
VkPipeline      g_lastBoundPipeline   = VK_NULL_HANDLE;
bool            g_viewportDirty       = true;
bool            g_scissorDirty        = true;

// Dummy white texture for when no texture is bound
VkImage g_dummyImage = VK_NULL_HANDLE;
VkDeviceMemory g_dummyMemory = VK_NULL_HANDLE;
VkImageView g_dummyImageView = VK_NULL_HANDLE;
VkSampler g_dummySampler = VK_NULL_HANDLE;

// Texture management
struct TextureData
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    int width = 0, height = 0;

    GLenum minFilter = GL_NEAREST;
    GLenum magFilter = GL_NEAREST;
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    bool samplerDirty = true;
    VkSampler sampler = VK_NULL_HANDLE;
};

std::unordered_map<GLuint, TextureData>& g_textures()
{
    static std::unordered_map<GLuint, TextureData> inst;
    return inst;
}
GLuint& g_nextTextureId()
{
    static GLuint id = 1;
    return id;
}

// VBO management
struct VBOData
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
    size_t size = 0;
    bool isStream = false; // true for GL_STREAM_DRAW (Tesselator ring buffer), false for GL_STATIC_DRAW (chunk VBOs)
};

std::unordered_map<GLuint, VBOData>& g_vbos()
{
    static std::unordered_map<GLuint, VBOData> inst;
    return inst;
}
GLuint& g_nextVBOId()
{
    static GLuint id = 1;
    return id;
}

// Display list management
enum class DLCmd : uint8_t
{
    Draw,
    PushMatrix,
    PopMatrix,
    Translate,
    Scale,
    Rotate,
    Color3f,
    Color4f,
    Normal3f,
    LoadIdentity,
    MultMatrix,
    EnableClientState,
    DisableClientState,
    VertexPointer,
    TexCoordPointer,
    ColorPointer,
    NormalPointer,
    BindBuffer,
    BufferData,
    Enable,
    Disable,
    BindTexture,
};

struct DLDrawData
{
    VkBuffer vbo;
    VkDeviceMemory vboMemory;
    GLenum mode;
    int first;
    int count;
    bool hasTexture;
    bool hasColor;
    bool hasNormal;
    int stride;
};

struct DLCommand
{
    DLCmd type;
    union
    {
        struct { float x, y, z; } translate;
        struct { float x, y, z; } scaleData;
        struct { float angle, x, y, z; } rotateData;
        struct { float r, g, b, a; } color;
        struct { float x, y, z; } normalData;
        DLDrawData draw;
        float matrix[16];
        GLenum cap;
        GLuint textureId;
    };
};

struct DisplayList
{
    std::vector<DLCommand> commands;
    bool valid = false;
};

std::unordered_map<GLuint, DisplayList>& g_displayLists()
{
    static std::unordered_map<GLuint, DisplayList> inst;
    return inst;
}
GLuint& g_nextListId()
{
    static GLuint id = 1;
    return id;
}

bool g_recording = false;
GLuint g_recordingListId = 0;
DisplayList* g_recordingList = nullptr;

// Pixel store state
int g_packAlignment = 4;
int g_unpackAlignment = 4;

// ============================================================================
// Vulkan Helpers
// ============================================================================

static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                          VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBufferCreateInfo bufCI = {};
    bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size = size;
    bufCI.usage = usage;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(Vulkan_Shared::getDevice(), &bufCI, nullptr, &buffer);

    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(Vulkan_Shared::getDevice(), buffer, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = Vulkan_Shared::findMemoryType(memReqs.memoryTypeBits, properties);

    vkAllocateMemory(Vulkan_Shared::getDevice(), &allocInfo, nullptr, &memory);
    vkBindBufferMemory(Vulkan_Shared::getDevice(), buffer, memory, 0);
}

static VkCommandBuffer beginOneShotCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Vulkan_Shared::getCommandPool();
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cb;
    vkAllocateCommandBuffers(Vulkan_Shared::getDevice(), &allocInfo, &cb);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cb, &beginInfo);

    return cb;
}

static void endOneShotCommands(VkCommandBuffer cb)
{
    vkEndCommandBuffer(cb);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb;

    // Use a fence instead of vkQueueWaitIdle to avoid draining the entire GPU pipeline
    VkFenceCreateInfo fenceCI = {};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    vkCreateFence(Vulkan_Shared::getDevice(), &fenceCI, nullptr, &fence);

    vkQueueSubmit(Vulkan_Shared::getGraphicsQueue(), 1, &submitInfo, fence);
    vkWaitForFences(Vulkan_Shared::getDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(Vulkan_Shared::getDevice(), fence, nullptr);
    vkFreeCommandBuffers(Vulkan_Shared::getDevice(), Vulkan_Shared::getCommandPool(), 1, &cb);
}

// Record a layout transition barrier into an existing command buffer (no submit)
static void recordImageBarrier(VkCommandBuffer cb, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                               VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage, dstStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else
    {
        barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(cb, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

// Standalone transition (one-shot command buffer) — used only during init
static void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT)
{
    VkCommandBuffer cb = beginOneShotCommands();
    recordImageBarrier(cb, image, oldLayout, newLayout, aspectMask);
    endOneShotCommands(cb);
}

// ============================================================================
// Pipeline Management
// ============================================================================

static VkCompareOp mapDepthFunc(GLenum gl)
{
    switch (gl)
    {
        case GL_NEVER: return VK_COMPARE_OP_NEVER;
        case GL_LESS: return VK_COMPARE_OP_LESS;
        case GL_EQUAL: return VK_COMPARE_OP_EQUAL;
        case GL_LEQUAL: return VK_COMPARE_OP_LESS_OR_EQUAL;
        case GL_GREATER: return VK_COMPARE_OP_GREATER;
        case GL_NOTEQUAL: return VK_COMPARE_OP_NOT_EQUAL;
        case GL_GEQUAL: return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case GL_ALWAYS: return VK_COMPARE_OP_ALWAYS;
        default: return VK_COMPARE_OP_LESS;
    }
}

static VkBlendFactor mapBlendFactor(GLenum gl)
{
    switch (gl)
    {
        case GL_ZERO: return VK_BLEND_FACTOR_ZERO;
        case GL_ONE: return VK_BLEND_FACTOR_ONE;
        case GL_SRC_ALPHA: return VK_BLEND_FACTOR_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case GL_DST_COLOR: return VK_BLEND_FACTOR_DST_COLOR;
        case GL_SRC_COLOR: return VK_BLEND_FACTOR_SRC_COLOR;
        default: return VK_BLEND_FACTOR_ONE;
    }
}

static VkPrimitiveTopology mapPrimitive(GLenum mode)
{
    switch (mode)
    {
        case GL_TRIANGLES: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case GL_TRIANGLE_STRIP: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case GL_LINES: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case GL_LINE_STRIP: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case GL_QUADS: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

static uint8_t blendFactorIndex(GLenum f)
{
    switch (f)
    {
        case GL_ZERO: return 0;
        case GL_ONE: return 1;
        case GL_SRC_ALPHA: return 2;
        case GL_ONE_MINUS_SRC_ALPHA: return 3;
        case GL_DST_ALPHA: return 4;
        case GL_ONE_MINUS_DST_ALPHA: return 5;
        case GL_SRC_COLOR: return 6;
        case GL_ONE_MINUS_SRC_COLOR: return 7;
        case GL_DST_COLOR: return 8;
        case GL_ONE_MINUS_DST_COLOR: return 9;
        default: return 15;
    }
}

static uint8_t depthFuncIndex(GLenum f)
{
    switch (f)
    {
        case GL_NEVER: return 0;
        case GL_LESS: return 1;
        case GL_EQUAL: return 2;
        case GL_LEQUAL: return 3;
        case GL_GREATER: return 4;
        case GL_NOTEQUAL: return 5;
        case GL_GEQUAL: return 6;
        case GL_ALWAYS: return 7;
        default: return 7;
    }
}

static uint64_t makePipelineKey()
{
    uint64_t key = 0;
    key |= (uint64_t)state.blend;                              // bit 0      (1 bit)
    key |= (uint64_t)blendFactorIndex(state.blendSrc) << 1;    // bits 1-4   (4 bits)
    key |= (uint64_t)blendFactorIndex(state.blendDst) << 5;    // bits 5-8   (4 bits)
    key |= (uint64_t)state.depthTest << 9;                     // bit 9      (1 bit)
    key |= (uint64_t)state.depthWrite << 10;                   // bit 10     (1 bit)
    key |= (uint64_t)depthFuncIndex(state.depthFunc) << 11;    // bits 11-13 (3 bits)
    key |= (uint64_t)state.cullFace << 14;                     // bit 14     (1 bit)
    key |= (uint64_t)state.colorMask[0] << 15;                 // bit 15
    key |= (uint64_t)state.colorMask[1] << 16;                 // bit 16
    key |= (uint64_t)state.colorMask[2] << 17;                 // bit 17
    key |= (uint64_t)state.colorMask[3] << 18;                 // bit 18
    key |= (uint64_t)state.logicOp << 19;                      // bit 19
    key |= (uint64_t)state.polyOffsetFill << 20;               // bit 20
    return key;
}

static VkPipeline getOrCreatePipeline(VkPrimitiveTopology topology)
{
    uint64_t key = makePipelineKey();
    key |= (uint64_t)topology << 32;

    auto it = g_pipelineCache().find(key);
    if (it != g_pipelineCache().end())
        return it->second;

    auto device = Vulkan_Shared::getDevice();

    // Shader stages
    VkPipelineShaderStageCreateInfo shaderStages[2] = {};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = g_vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = g_fragShaderModule;
    shaderStages[1].pName = "main";

    // Vertex input (32-byte Tesselator format)
    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = 32;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDescs[4] = {};
    // Position: float3 at offset 0
    attrDescs[0].binding = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[0].offset = 0;
    // TexCoord: float2 at offset 12
    attrDescs[1].binding = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[1].offset = 12;
    // Color: ubyte4 UNORM at offset 20
    attrDescs[2].binding = 0;
    attrDescs[2].location = 2;
    attrDescs[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    attrDescs[2].offset = 20;
    // Normal: byte4 SNORM at offset 24
    attrDescs[3].binding = 0;
    attrDescs[3].location = 3;
    attrDescs[3].format = VK_FORMAT_R8G8B8A8_SNORM;
    attrDescs[3].offset = 24;

    VkPipelineVertexInputStateCreateInfo vertexInputCI = {};
    vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCI.vertexBindingDescriptionCount = 1;
    vertexInputCI.pVertexBindingDescriptions = &bindingDesc;
    vertexInputCI.vertexAttributeDescriptionCount = 4;
    vertexInputCI.pVertexAttributeDescriptions = attrDescs;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo iaCI = {};
    iaCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaCI.topology = topology;
    iaCI.primitiveRestartEnable = VK_FALSE;

    // Viewport and scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewportCI = {};
    viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportCI.viewportCount = 1;
    viewportCI.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterCI = {};
    rasterCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterCI.depthClampEnable = VK_FALSE;
    rasterCI.rasterizerDiscardEnable = VK_FALSE;
    rasterCI.polygonMode = VK_POLYGON_MODE_FILL;
    rasterCI.lineWidth = 1.0f;
    rasterCI.cullMode = state.cullFace ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    // CCW front face — matches OpenGL convention (Y-flip is in projection, not viewport)
    rasterCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterCI.depthBiasEnable = state.polyOffsetFill ? VK_TRUE : VK_FALSE;

    // Multisample
    VkPipelineMultisampleStateCreateInfo msCI = {};
    msCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo dsCI = {};
    dsCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsCI.depthTestEnable = state.depthTest ? VK_TRUE : VK_FALSE;
    dsCI.depthWriteEnable = state.depthWrite ? VK_TRUE : VK_FALSE;
    dsCI.depthCompareOp = mapDepthFunc(state.depthFunc);
    dsCI.depthBoundsTestEnable = VK_FALSE;
    dsCI.stencilTestEnable = VK_FALSE;

    // Color blend
    VkPipelineColorBlendAttachmentState cbAttachment = {};
    cbAttachment.colorWriteMask = 0;
    if (state.colorMask[0]) cbAttachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
    if (state.colorMask[1]) cbAttachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
    if (state.colorMask[2]) cbAttachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
    if (state.colorMask[3]) cbAttachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;

    if (state.logicOp)
    {
        cbAttachment.blendEnable = VK_FALSE;
    }
    else
    {
        cbAttachment.blendEnable = state.blend ? VK_TRUE : VK_FALSE;
        cbAttachment.srcColorBlendFactor = mapBlendFactor(state.blendSrc);
        cbAttachment.dstColorBlendFactor = mapBlendFactor(state.blendDst);
        cbAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        cbAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cbAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cbAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    VkPipelineColorBlendStateCreateInfo cbCI = {};
    cbCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    if (state.logicOp)
    {
        cbCI.logicOpEnable = VK_TRUE;
        cbCI.logicOp = VK_LOGIC_OP_OR_REVERSE;
    }
    cbCI.attachmentCount = 1;
    cbCI.pAttachments = &cbAttachment;

    // Dynamic state
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BIAS,
    };
    VkPipelineDynamicStateCreateInfo dynCI = {};
    dynCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynCI.dynamicStateCount = 3;
    dynCI.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineCI = {};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.stageCount = 2;
    pipelineCI.pStages = shaderStages;
    pipelineCI.pVertexInputState = &vertexInputCI;
    pipelineCI.pInputAssemblyState = &iaCI;
    pipelineCI.pViewportState = &viewportCI;
    pipelineCI.pRasterizationState = &rasterCI;
    pipelineCI.pMultisampleState = &msCI;
    pipelineCI.pDepthStencilState = &dsCI;
    pipelineCI.pColorBlendState = &cbCI;
    pipelineCI.pDynamicState = &dynCI;
    pipelineCI.layout = g_pipelineLayout;
    pipelineCI.renderPass = Vulkan_Shared::getRenderPass();
    pipelineCI.subpass = 0;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline) != VK_SUCCESS)
    {
        std::cerr << "Failed to create graphics pipeline (key=" << key << ")" << std::endl;
        return VK_NULL_HANDLE;
    }

    g_pipelineCache()[key] = pipeline;
    return pipeline;
}

// ============================================================================
// Sampler Management
// ============================================================================

static VkSampler getOrCreateSampler(TextureData& tex)
{
    if (!tex.samplerDirty && tex.sampler != VK_NULL_HANDLE)
        return tex.sampler;

    // Build cache key from the actual sampler parameters (not raw GL enums)
    // to avoid bit collisions between filter enum values and wrap mode bits.
    bool minNearest = (tex.minFilter == GL_NEAREST || tex.minFilter == GL_NEAREST_MIPMAP_NEAREST);
    bool magNearest = (tex.magFilter == GL_NEAREST);
    uint32_t key = 0;
    key |= (uint32_t)minNearest;         // bit 0
    key |= (uint32_t)magNearest << 1;    // bit 1
    key |= (uint32_t)(tex.wrapS == GL_CLAMP ? 1 : 0) << 2; // bit 2
    key |= (uint32_t)(tex.wrapT == GL_CLAMP ? 1 : 0) << 3; // bit 3

    auto it = g_samplerCache().find(key);
    if (it != g_samplerCache().end())
    {
        tex.sampler = it->second;
        tex.samplerDirty = false;
        return tex.sampler;
    }

    VkSamplerCreateInfo samplerCI = {};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

    samplerCI.minFilter = minNearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerCI.magFilter = magNearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    samplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    samplerCI.addressModeU = (tex.wrapS == GL_CLAMP) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = (tex.wrapT == GL_CLAMP) ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.maxLod = VK_LOD_CLAMP_NONE;

    VkSampler sampler;
    vkCreateSampler(Vulkan_Shared::getDevice(), &samplerCI, nullptr, &sampler);

    g_samplerCache()[key] = sampler;
    tex.sampler = sampler;
    tex.samplerDirty = false;
    return sampler;
}

// ============================================================================
// Initialization
// ============================================================================

static void createDummyTexture()
{
    auto device = Vulkan_Shared::getDevice();

    // Create 1x1 white image
    VkImageCreateInfo imageCI = {};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCI.extent = {1, 1, 1};
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    vkCreateImage(device, &imageCI, nullptr, &g_dummyImage);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, g_dummyImage, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = Vulkan_Shared::findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &g_dummyMemory);
    vkBindImageMemory(device, g_dummyImage, g_dummyMemory, 0);

    // Upload white pixel
    VkBuffer stagingBuf;
    VkDeviceMemory stagingMem;
    createBuffer(4, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuf, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, 4, 0, &data);
    uint32_t white = 0xFFFFFFFF;
    memcpy(data, &white, 4);
    vkUnmapMemory(device, stagingMem);

    transitionImageLayout(g_dummyImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkCommandBuffer cb = beginOneShotCommands();
    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {1, 1, 1};
    vkCmdCopyBufferToImage(cb, stagingBuf, g_dummyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    endOneShotCommands(cb);

    transitionImageLayout(g_dummyImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuf, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);

    // Create image view
    VkImageViewCreateInfo viewCI = {};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.image = g_dummyImage;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewCI, nullptr, &g_dummyImageView);

    // Create sampler
    VkSamplerCreateInfo samplerCI = {};
    samplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCI.minFilter = VK_FILTER_NEAREST;
    samplerCI.magFilter = VK_FILTER_NEAREST;
    samplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    vkCreateSampler(device, &samplerCI, nullptr, &g_dummySampler);
}

static void initVulkanResources()
{
    auto device = Vulkan_Shared::getDevice();
    if (!device) return;

    // Get UBO alignment
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(Vulkan_Shared::getPhysicalDevice(), &props);
    g_uboAlignment = (uint32_t)props.limits.minUniformBufferOffsetAlignment;
    if (g_uboAlignment < 16) g_uboAlignment = 16;

    // Create shader modules
    {
        VkShaderModuleCreateInfo ci = {};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = g_vertShaderCodeSize;
        ci.pCode = g_vertShaderCode;
        vkCreateShaderModule(device, &ci, nullptr, &g_vertShaderModule);

        ci.codeSize = g_fragShaderCodeSize;
        ci.pCode = g_fragShaderCode;
        vkCreateShaderModule(device, &ci, nullptr, &g_fragShaderModule);
    }

    // Descriptor set layout: binding 0 = UBO (dynamic), binding 1 = combined image sampler
    {
        VkDescriptorSetLayoutBinding bindings[2] = {};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutCI = {};
        layoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutCI.bindingCount = 2;
        layoutCI.pBindings = bindings;

        vkCreateDescriptorSetLayout(device, &layoutCI, nullptr, &g_descriptorSetLayout);
    }

    // Pipeline layout
    {
        VkPipelineLayoutCreateInfo layoutCI = {};
        layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.setLayoutCount = 1;
        layoutCI.pSetLayouts = &g_descriptorSetLayout;

        vkCreatePipelineLayout(device, &layoutCI, nullptr, &g_pipelineLayout);
    }

    // Per-frame resources
    for (int i = 0; i < MAX_FRAMES; i++)
    {
        auto& fr = g_frames[i];

        // UBO buffer (persistently mapped)
        createBuffer(UBO_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     fr.uboBuffer, fr.uboMemory);
        vkMapMemory(device, fr.uboMemory, 0, UBO_SIZE, 0, &fr.uboMapped);

        // Dynamic VB (persistently mapped)
        createBuffer(DYNAMIC_VB_SIZE, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     fr.dynamicVB, fr.dynamicVBMemory);
        vkMapMemory(device, fr.dynamicVBMemory, 0, DYNAMIC_VB_SIZE, 0, &fr.dynamicVBMapped);

        // Descriptor pool
        VkDescriptorPoolSize poolSizes[2] = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSizes[0].descriptorCount = MAX_DESCRIPTOR_SETS;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = MAX_DESCRIPTOR_SETS;

        VkDescriptorPoolCreateInfo poolCI = {};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.maxSets = MAX_DESCRIPTOR_SETS;
        poolCI.poolSizeCount = 2;
        poolCI.pPoolSizes = poolSizes;

        vkCreateDescriptorPool(device, &poolCI, nullptr, &fr.descriptorPool);

        // Pre-allocate all descriptor sets
        fr.descriptorSets.resize(MAX_DESCRIPTOR_SETS);
        std::vector<VkDescriptorSetLayout> layouts(MAX_DESCRIPTOR_SETS, g_descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = fr.descriptorPool;
        allocInfo.descriptorSetCount = MAX_DESCRIPTOR_SETS;
        allocInfo.pSetLayouts = layouts.data();

        vkAllocateDescriptorSets(device, &allocInfo, fr.descriptorSets.data());
    }

    createDummyTexture();

    g_initialized = true;
}

static void ensureInit()
{
    if (!g_initialized)
        initVulkanResources();
}

// ============================================================================
// Render Pass Management
// ============================================================================

static void ensureRenderPass()
{
    if (Vulkan_Shared::isRenderPassActive())
        return;

    auto cb = Vulkan_Shared::getCurrentCommandBuffer();
    if (cb == VK_NULL_HANDLE) return; // Frame acquire failed (e.g. window minimized)
    auto imageIndex = Vulkan_Shared::getCurrentImageIndex();

    VkClearValue clearValues[2] = {};
    clearValues[0].color = {{state.clearColor[0], state.clearColor[1], state.clearColor[2], state.clearColor[3]}};
    clearValues[1].depthStencil = {(float)state.clearDepth, 0};

    VkRenderPassBeginInfo rpBI = {};
    rpBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBI.renderPass = Vulkan_Shared::getRenderPass();
    rpBI.framebuffer = Vulkan_Shared::getSwapchainFramebuffers()[imageIndex];
    rpBI.renderArea.extent = Vulkan_Shared::getSwapchainExtent();
    rpBI.clearValueCount = 2;
    rpBI.pClearValues = clearValues;

    vkCmdBeginRenderPass(cb, &rpBI, VK_SUBPASS_CONTENTS_INLINE);
    Vulkan_Shared::setRenderPassActive(true);

    // Dynamic state is invalidated after vkCmdBeginRenderPass
    g_viewportDirty = true;
    g_scissorDirty  = true;
    g_lastBoundPipeline = VK_NULL_HANDLE;
}

// ============================================================================
// State Flush (core draw-call setup)
// ============================================================================

static uint32_t alignUp(uint32_t value, uint32_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

static void resetFrameIfNeeded()
{
    int frame = Vulkan_Shared::getCurrentFrame();
    if (frame != g_lastResetFrame)
    {
        g_lastResetFrame = frame;
        auto& fr = g_frames[frame];
        fr.uboOffset = 0;
        fr.dynamicVBOffset = 0;
        fr.descriptorSetIndex = 0;

        // Invalidate per-frame state tracking
        g_lastBoundImageView  = VK_NULL_HANDLE;
        g_lastBoundSampler    = VK_NULL_HANDLE;
        g_lastDescriptorSet   = VK_NULL_HANDLE;
        g_lastBoundPipeline   = VK_NULL_HANDLE;
        g_viewportDirty       = true;
        g_scissorDirty        = true;

        // Flush deferred buffer deletions — GPU is done with this frame slot
        auto device = Vulkan_Shared::getDevice();
        for (auto& db : fr.pendingDeletes)
        {
            vkDestroyBuffer(device, db.buffer, nullptr);
            vkFreeMemory(device, db.memory, nullptr);
        }
        fr.pendingDeletes.clear();
    }
}

static void flushState(bool hasTexCoord, bool hasColor, bool hasNormal)
{
    ensureInit();
    resetFrameIfNeeded();
    ensureRenderPass();

    auto cb = Vulkan_Shared::getCurrentCommandBuffer();
    if (cb == VK_NULL_HANDLE) return; // Frame acquire failed
    int frame = Vulkan_Shared::getCurrentFrame();
    auto& fr = g_frames[frame];

    // Fill CBData — NO transpose for Vulkan (GLSL uses column-major natively)
    CBData cbData = {};

    float mvp[16];
    mat4::multiply(mvp, projStack.current(), mvStack.current());
    memcpy(cbData.mvp, mvp, 64);
    memcpy(cbData.mv, mvStack.current(), 64);
    memcpy(cbData.texMat, texStack.current(), 64);

    cbData.lightingEnabled = state.lighting ? 1 : 0;
    memcpy(cbData.lightDir0, state.lightDir[0], 16);
    cbData.lightDir0[3] = state.light0 ? 1.0f : 0.0f;
    memcpy(cbData.lightDir1, state.lightDir[1], 16);
    cbData.lightDir1[3] = state.light1 ? 1.0f : 0.0f;
    memcpy(cbData.lightDiffuse0, state.lightDiffuse[0], 16);
    memcpy(cbData.lightDiffuse1, state.lightDiffuse[1], 16);
    memcpy(cbData.globalAmbient, state.globalAmbient, 16);

    memcpy(cbData.currentColor, state.color, 16);
    cbData.currentNormal[0] = state.normal[0];
    cbData.currentNormal[1] = state.normal[1];
    cbData.currentNormal[2] = state.normal[2];
    cbData.currentNormal[3] = 0.0f;

    cbData.textureEnabled = (state.texture2D && state.boundTexture != 0) ? 1 : 0;
    cbData.hasVertexColor = hasColor ? 1 : 0;
    cbData.hasVertexNormal = hasNormal ? 1 : 0;
    cbData.hasVertexTexCoord = hasTexCoord ? 1 : 0;

    memcpy(cbData.fogColor, state.fogColor, 16);
    cbData.fogStart = state.fogStart;
    cbData.fogEnd = state.fogEnd;
    cbData.fogDensity = state.fogDensity;
    if (!state.fog)
        cbData.fogMode = 0;
    else if (state.fogMode == GL_LINEAR)
        cbData.fogMode = 1;
    else
        cbData.fogMode = 2;

    cbData.alphaRef = state.alphaRef;
    cbData.alphaTestEnabled = state.alphaTest ? 1 : 0;

    // Sub-allocate UBO
    uint32_t uboAlignedSize = alignUp((uint32_t)sizeof(CBData), g_uboAlignment);
    if (fr.uboOffset + uboAlignedSize > UBO_SIZE)
    {
        std::cerr << "Vulkan: UBO overflow!" << std::endl;
        return;
    }
    uint32_t uboOffset = fr.uboOffset;
    fr.uboOffset += uboAlignedSize;

    memcpy((uint8_t*)fr.uboMapped + uboOffset, &cbData, sizeof(CBData));

    // Resolve current texture state
    VkImageView curImageView;
    VkSampler curSampler;

    if (state.texture2D && state.boundTexture != 0)
    {
        auto it = g_textures().find(state.boundTexture);
        if (it != g_textures().end() && it->second.imageView != VK_NULL_HANDLE)
        {
            curImageView = it->second.imageView;
            curSampler = getOrCreateSampler(it->second);
        }
        else
        {
            curImageView = g_dummyImageView;
            curSampler = g_dummySampler;
        }
    }
    else
    {
        curImageView = g_dummyImageView;
        curSampler = g_dummySampler;
    }

    // Only allocate and update a new descriptor set when the texture binding changes.
    // The UBO uses dynamic offsets, so different UBO data doesn't require a new set.
    if (curImageView != g_lastBoundImageView || curSampler != g_lastBoundSampler || g_lastDescriptorSet == VK_NULL_HANDLE)
    {
        if (fr.descriptorSetIndex >= MAX_DESCRIPTOR_SETS)
        {
            std::cerr << "Vulkan: descriptor set overflow!" << std::endl;
            return;
        }

        VkDescriptorSet ds = fr.descriptorSets[fr.descriptorSetIndex++];

        VkDescriptorBufferInfo bufInfo = {};
        bufInfo.buffer = fr.uboBuffer;
        bufInfo.offset = 0;
        bufInfo.range = sizeof(CBData);

        VkDescriptorImageInfo imgInfo = {};
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgInfo.imageView = curImageView;
        imgInfo.sampler = curSampler;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = ds;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writes[0].pBufferInfo = &bufInfo;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = ds;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &imgInfo;

        vkUpdateDescriptorSets(Vulkan_Shared::getDevice(), 2, writes, 0, nullptr);

        g_lastDescriptorSet   = ds;
        g_lastBoundImageView  = curImageView;
        g_lastBoundSampler    = curSampler;
    }

    // Bind descriptor set with dynamic UBO offset (always needed — offset changes per draw)
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipelineLayout,
                            0, 1, &g_lastDescriptorSet, 1, &uboOffset);

    // Viewport — only set when changed
    if (g_viewportDirty)
    {
        VkViewport vp = {};
        int bbHeight = Vulkan_Shared::getBackbufferHeight();
        vp.x = (float)state.viewport[0];
        vp.y = (float)(bbHeight - state.viewport[1] - state.viewport[3]);
        vp.width = (float)state.viewport[2];
        vp.height = (float)state.viewport[3];
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport(cb, 0, 1, &vp);
        g_viewportDirty = false;
    }

    // Scissor — only set once per frame
    if (g_scissorDirty)
    {
        VkRect2D scissor = {};
        scissor.extent = Vulkan_Shared::getSwapchainExtent();
        vkCmdSetScissor(cb, 0, 1, &scissor);
        g_scissorDirty = false;
    }

    // Depth bias
    if (state.polyOffsetFill)
        vkCmdSetDepthBias(cb, state.polyOffsetUnits, 0.0f, state.polyOffsetFactor);
    else
        vkCmdSetDepthBias(cb, 0.0f, 0.0f, 0.0f);
}

} // anonymous namespace

// ============================================================================
// GL Function Implementations
// ============================================================================

// --- Matrix Operations ---

void glMatrixMode(GLenum mode)
{
    currentMatrixMode = mode;
}

void glLoadIdentity()
{
    getActiveStack().loadIdentity();
}

void glPushMatrix()
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::PushMatrix;
        g_recordingList->commands.push_back(cmd);
        return;
    }
    getActiveStack().push();
}

void glPopMatrix()
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::PopMatrix;
        g_recordingList->commands.push_back(cmd);
        return;
    }
    getActiveStack().pop();
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Translate;
        cmd.translate = {x, y, z};
        g_recordingList->commands.push_back(cmd);
        return;
    }
    mat4::translate(getActiveStack().current(), x, y, z);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Rotate;
        cmd.rotateData = {angle, x, y, z};
        g_recordingList->commands.push_back(cmd);
        return;
    }
    mat4::rotate(getActiveStack().current(), angle, x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Scale;
        cmd.scaleData = {x, y, z};
        g_recordingList->commands.push_back(cmd);
        return;
    }
    mat4::scale(getActiveStack().current(), x, y, z);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    glScalef((float)x, (float)y, (float)z);
}

void glMultMatrixf(const GLfloat* m)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::MultMatrix;
        memcpy(cmd.matrix, m, 64);
        g_recordingList->commands.push_back(cmd);
        return;
    }
    float tmp[16];
    mat4::multiply(tmp, getActiveStack().current(), m);
    mat4::copy(getActiveStack().current(), tmp);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    mat4::ortho(getActiveStack().current(), left, right, bottom, top, zNear, zFar);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    mat4::frustum(getActiveStack().current(), left, right, bottom, top, zNear, zFar);
}

// --- Enable/Disable ---

static void setCapability(GLenum cap, bool enabled)
{
    switch (cap)
    {
        case GL_TEXTURE_2D: state.texture2D = enabled; break;
        case GL_BLEND: state.blend = enabled; break;
        case GL_ALPHA_TEST: state.alphaTest = enabled; break;
        case GL_DEPTH_TEST: state.depthTest = enabled; break;
        case GL_CULL_FACE: state.cullFace = enabled; break;
        case GL_FOG: state.fog = enabled; break;
        case GL_LIGHTING: state.lighting = enabled; break;
        case GL_LIGHT0: state.light0 = enabled; break;
        case GL_LIGHT1: state.light1 = enabled; break;
        case GL_COLOR_MATERIAL: state.colorMaterial = enabled; break;
        case GL_POLYGON_OFFSET_FILL: state.polyOffsetFill = enabled; break;
        case GL_COLOR_LOGIC_OP: state.logicOp = enabled; break;
        case GL_RESCALE_NORMAL: state.rescaleNormal = enabled; break;
        case GL_NORMALIZE: state.normalizeEnabled = enabled; break;
        case GL_VERTEX_ARRAY: state.vertexArray = enabled; break;
        case GL_TEXTURE_COORD_ARRAY: state.texCoordArray = enabled; break;
        case GL_COLOR_ARRAY: state.colorArray = enabled; break;
        case GL_NORMAL_ARRAY: state.normalArray = enabled; break;
        default: break;
    }
}

void glEnable(GLenum cap)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Enable;
        cmd.cap = cap;
        g_recordingList->commands.push_back(cmd);
        return;
    }
    setCapability(cap, true);
}

void glDisable(GLenum cap)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Disable;
        cmd.cap = cap;
        g_recordingList->commands.push_back(cmd);
        return;
    }
    setCapability(cap, false);
}

// --- Blend ---

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    state.blendSrc = sfactor;
    state.blendDst = dfactor;
}

// --- Color ---

void glColor3f(GLfloat r, GLfloat g, GLfloat b)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Color3f;
        cmd.color = {r, g, b, state.color[3]};
        g_recordingList->commands.push_back(cmd);
        return;
    }
    state.color[0] = r;
    state.color[1] = g;
    state.color[2] = b;
}

void glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Color4f;
        cmd.color = {r, g, b, a};
        g_recordingList->commands.push_back(cmd);
        return;
    }
    state.color[0] = r;
    state.color[1] = g;
    state.color[2] = b;
    state.color[3] = a;
}

// --- Depth ---

void glDepthMask(GLboolean flag)
{
    state.depthWrite = (flag != 0);
}

void glDepthFunc(GLenum func)
{
    state.depthFunc = func;
}

void glClearDepth(GLdouble depth)
{
    state.clearDepth = depth;
}

// --- Alpha Test ---

void glAlphaFunc(GLenum func, GLclampf ref)
{
    state.alphaFunc = func;
    state.alphaRef = ref;
}

// --- Fog ---

void glFogf(GLenum pname, GLfloat param)
{
    switch (pname)
    {
        case GL_FOG_DENSITY: state.fogDensity = param; break;
        case GL_FOG_START: state.fogStart = param; break;
        case GL_FOG_END: state.fogEnd = param; break;
    }
}

void glFogfv(GLenum pname, const GLfloat* params)
{
    if (pname == GL_FOG_COLOR)
        memcpy(state.fogColor, params, 16);
}

void glFogi(GLenum pname, GLint param)
{
    switch (pname)
    {
        case GL_FOG_MODE: state.fogMode = param; break;
        default: break;
    }
}

// --- Lighting ---

void glLightfv(GLenum light, GLenum pname, const GLfloat* params)
{
    int idx = (light == GL_LIGHT1) ? 1 : 0;
    switch (pname)
    {
        case GL_POSITION:
        {
            float transformed[4];
            mat4::transformVec4(transformed, mvStack.current(), params);
            if (transformed[3] == 0.0f)
            {
                float len = sqrtf(transformed[0] * transformed[0] + transformed[1] * transformed[1] + transformed[2] * transformed[2]);
                if (len > 1e-8f)
                {
                    transformed[0] /= len;
                    transformed[1] /= len;
                    transformed[2] /= len;
                }
            }
            memcpy(state.lightDir[idx], transformed, 16);
            break;
        }
        case GL_DIFFUSE:
            memcpy(state.lightDiffuse[idx], params, 16);
            break;
        case GL_AMBIENT:
            memcpy(state.lightAmbient[idx], params, 16);
            break;
        case GL_SPECULAR:
            break;
    }
}

void glLightModelfv(GLenum pname, const GLfloat* params)
{
    if (pname == GL_LIGHT_MODEL_AMBIENT)
        memcpy(state.globalAmbient, params, 16);
}

void glColorMaterial(GLenum face, GLenum mode)
{
    (void)face;
    (void)mode;
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::Normal3f;
        cmd.normalData = {nx, ny, nz};
        g_recordingList->commands.push_back(cmd);
        return;
    }
    state.normal[0] = nx;
    state.normal[1] = ny;
    state.normal[2] = nz;
}

void glShadeModel(GLenum mode)
{
    state.shadeModel = mode;
}

// --- Clear ---

void glClear(GLbitfield mask)
{
    ensureInit();
    resetFrameIfNeeded();
    ensureRenderPass();

    auto cb = Vulkan_Shared::getCurrentCommandBuffer();
    if (cb == VK_NULL_HANDLE) return; // Frame acquire failed
    VkClearAttachment clearAttachments[2] = {};
    uint32_t clearCount = 0;

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        clearAttachments[clearCount].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearAttachments[clearCount].colorAttachment = 0;
        clearAttachments[clearCount].clearValue.color = {{state.clearColor[0], state.clearColor[1], state.clearColor[2], state.clearColor[3]}};
        clearCount++;
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        clearAttachments[clearCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        clearAttachments[clearCount].clearValue.depthStencil = {(float)state.clearDepth, 0};
        clearCount++;
    }

    if (clearCount > 0)
    {
        VkClearRect clearRect = {};
        clearRect.rect.extent = Vulkan_Shared::getSwapchainExtent();
        clearRect.baseArrayLayer = 0;
        clearRect.layerCount = 1;

        vkCmdClearAttachments(cb, clearCount, clearAttachments, 1, &clearRect);
    }
}

void glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
    state.clearColor[0] = r;
    state.clearColor[1] = g;
    state.clearColor[2] = b;
    state.clearColor[3] = a;
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    state.viewport[0] = x;
    state.viewport[1] = y;
    state.viewport[2] = width;
    state.viewport[3] = height;
    g_viewportDirty = true;
}

// --- Textures ---

void glGenTextures(GLsizei n, GLuint* textures)
{
    for (int i = 0; i < n; i++)
    {
        textures[i] = g_nextTextureId()++;
        g_textures()[textures[i]] = TextureData();
    }
}

void glDeleteTextures(GLsizei n, const GLuint* textures)
{
    auto device = Vulkan_Shared::getDevice();
    for (int i = 0; i < n; i++)
    {
        auto it = g_textures().find(textures[i]);
        if (it != g_textures().end())
        {
            if (it->second.imageView != VK_NULL_HANDLE)
                vkDestroyImageView(device, it->second.imageView, nullptr);
            if (it->second.image != VK_NULL_HANDLE)
                vkDestroyImage(device, it->second.image, nullptr);
            if (it->second.memory != VK_NULL_HANDLE)
                vkFreeMemory(device, it->second.memory, nullptr);
            g_textures().erase(it);
        }
    }
}

void glBindTexture(GLenum target, GLuint texture)
{
    (void)target;
    if (g_recording)
    {
        DLCommand cmd;
        cmd.type = DLCmd::BindTexture;
        cmd.textureId = texture;
        g_recordingList->commands.push_back(cmd);
    }
    state.boundTexture = texture;
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    (void)target;
    auto it = g_textures().find(state.boundTexture);
    if (it == g_textures().end()) return;

    switch (pname)
    {
        case GL_TEXTURE_MIN_FILTER: it->second.minFilter = param; it->second.samplerDirty = true; break;
        case GL_TEXTURE_MAG_FILTER: it->second.magFilter = param; it->second.samplerDirty = true; break;
        case GL_TEXTURE_WRAP_S: it->second.wrapS = param; it->second.samplerDirty = true; break;
        case GL_TEXTURE_WRAP_T: it->second.wrapT = param; it->second.samplerDirty = true; break;
    }
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
                  GLint border, GLenum format, GLenum type, const void* pixels)
{
    (void)target; (void)level; (void)internalformat; (void)border; (void)format; (void)type;

    auto it = g_textures().find(state.boundTexture);
    if (it == g_textures().end()) return;

    auto device = Vulkan_Shared::getDevice();

    // Release old
    if (it->second.imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, it->second.imageView, nullptr);
        it->second.imageView = VK_NULL_HANDLE;
    }
    if (it->second.image != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, it->second.image, nullptr);
        it->second.image = VK_NULL_HANDLE;
    }
    if (it->second.memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, it->second.memory, nullptr);
        it->second.memory = VK_NULL_HANDLE;
    }

    // Create image
    VkImageCreateInfo imageCI = {};
    imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCI.imageType = VK_IMAGE_TYPE_2D;
    imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageCI.extent = {(uint32_t)width, (uint32_t)height, 1};
    imageCI.mipLevels = 1;
    imageCI.arrayLayers = 1;
    imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    vkCreateImage(device, &imageCI, nullptr, &it->second.image);

    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device, it->second.image, &memReqs);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReqs.size;
    allocInfo.memoryTypeIndex = Vulkan_Shared::findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(device, &allocInfo, nullptr, &it->second.memory);
    vkBindImageMemory(device, it->second.image, it->second.memory, 0);

    if (pixels)
    {
        // Upload via staging buffer — batch transition+copy+transition into one submit
        VkDeviceSize imageSize = width * height * 4;

        VkBuffer stagingBuf;
        VkDeviceMemory stagingMem;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingBuf, stagingMem);

        void* data;
        vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
        memcpy(data, pixels, imageSize);
        vkUnmapMemory(device, stagingMem);

        VkCommandBuffer cb = beginOneShotCommands();

        recordImageBarrier(cb, it->second.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
        vkCmdCopyBufferToImage(cb, stagingBuf, it->second.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        recordImageBarrier(cb, it->second.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        endOneShotCommands(cb);

        vkDestroyBuffer(device, stagingBuf, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
    }
    else
    {
        // No data — transition to shader read anyway
        transitionImageLayout(it->second.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    // Create image view
    VkImageViewCreateInfo viewCI = {};
    viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCI.image = it->second.image;
    viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCI.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCI.subresourceRange.levelCount = 1;
    viewCI.subresourceRange.layerCount = 1;
    vkCreateImageView(device, &viewCI, nullptr, &it->second.imageView);

    it->second.width = width;
    it->second.height = height;
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
    (void)target; (void)level; (void)format; (void)type;

    auto it = g_textures().find(state.boundTexture);
    if (it == g_textures().end() || it->second.image == VK_NULL_HANDLE || !pixels) return;

    auto device = Vulkan_Shared::getDevice();

    VkDeviceSize imageSize = width * height * 4;

    VkBuffer stagingBuf;
    VkDeviceMemory stagingMem;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuf, stagingMem);

    void* data;
    vkMapMemory(device, stagingMem, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(device, stagingMem);

    // Batch transition+copy+transition into one submit
    VkCommandBuffer cb = beginOneShotCommands();

    recordImageBarrier(cb, it->second.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region = {};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {xoffset, yoffset, 0};
    region.imageExtent = {(uint32_t)width, (uint32_t)height, 1};
    vkCmdCopyBufferToImage(cb, stagingBuf, it->second.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    recordImageBarrier(cb, it->second.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    endOneShotCommands(cb);

    vkDestroyBuffer(device, stagingBuf, nullptr);
    vkFreeMemory(device, stagingMem, nullptr);
}

void glPixelStorei(GLenum pname, GLint param)
{
    if (pname == GL_PACK_ALIGNMENT) g_packAlignment = param;
    if (pname == GL_UNPACK_ALIGNMENT) g_unpackAlignment = param;
}

// --- Display Lists ---

GLuint glGenLists(GLsizei range)
{
    GLuint base = g_nextListId();
    g_nextListId() += range;
    return base;
}

void glDeleteLists(GLuint list, GLsizei range)
{
    auto device = Vulkan_Shared::getDevice();
    for (GLsizei i = 0; i < range; i++)
    {
        auto it = g_displayLists().find(list + i);
        if (it != g_displayLists().end())
        {
            for (auto& cmd : it->second.commands)
            {
                if (cmd.type == DLCmd::Draw && cmd.draw.vbo != VK_NULL_HANDLE)
                {
                    vkDestroyBuffer(device, cmd.draw.vbo, nullptr);
                    if (cmd.draw.vboMemory != VK_NULL_HANDLE)
                        vkFreeMemory(device, cmd.draw.vboMemory, nullptr);
                }
            }
            g_displayLists().erase(it);
        }
    }
}

void glNewList(GLuint list, GLenum mode)
{
    (void)mode;
    g_recording = true;
    g_recordingListId = list;

    auto device = Vulkan_Shared::getDevice();
    auto it = g_displayLists().find(list);
    if (it != g_displayLists().end())
    {
        for (auto& cmd : it->second.commands)
        {
            if (cmd.type == DLCmd::Draw && cmd.draw.vbo != VK_NULL_HANDLE)
            {
                vkDestroyBuffer(device, cmd.draw.vbo, nullptr);
                if (cmd.draw.vboMemory != VK_NULL_HANDLE)
                    vkFreeMemory(device, cmd.draw.vboMemory, nullptr);
            }
        }
    }

    g_displayLists()[list] = DisplayList();
    g_recordingList = &g_displayLists()[list];
    g_recordingList->valid = true;
}

void glEndList()
{
    g_recording = false;
    g_recordingList = nullptr;
}

void glCallList(GLuint list)
{
    auto it = g_displayLists().find(list);
    if (it == g_displayLists().end() || !it->second.valid)
        return;

    for (auto& cmd : it->second.commands)
    {
        switch (cmd.type)
        {
            case DLCmd::Draw:
            {
                auto cb = Vulkan_Shared::getCurrentCommandBuffer();
                if (!cb || cmd.draw.vbo == VK_NULL_HANDLE) break;

                flushState(cmd.draw.hasTexture, cmd.draw.hasColor, cmd.draw.hasNormal);

                VkPipeline pipeline = getOrCreatePipeline(mapPrimitive(cmd.draw.mode));
                if (pipeline == VK_NULL_HANDLE) break;
                if (pipeline != g_lastBoundPipeline)
                {
                    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                    g_lastBoundPipeline = pipeline;
                }

                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cb, 0, 1, &cmd.draw.vbo, &offset);
                vkCmdDraw(cb, cmd.draw.count, 1, cmd.draw.first, 0);
                break;
            }
            case DLCmd::PushMatrix:
                getActiveStack().push();
                break;
            case DLCmd::PopMatrix:
                getActiveStack().pop();
                break;
            case DLCmd::Translate:
                mat4::translate(getActiveStack().current(), cmd.translate.x, cmd.translate.y, cmd.translate.z);
                break;
            case DLCmd::Scale:
                mat4::scale(getActiveStack().current(), cmd.scaleData.x, cmd.scaleData.y, cmd.scaleData.z);
                break;
            case DLCmd::Rotate:
                mat4::rotate(getActiveStack().current(), cmd.rotateData.angle, cmd.rotateData.x, cmd.rotateData.y, cmd.rotateData.z);
                break;
            case DLCmd::Color3f:
                state.color[0] = cmd.color.r;
                state.color[1] = cmd.color.g;
                state.color[2] = cmd.color.b;
                break;
            case DLCmd::Color4f:
                state.color[0] = cmd.color.r;
                state.color[1] = cmd.color.g;
                state.color[2] = cmd.color.b;
                state.color[3] = cmd.color.a;
                break;
            case DLCmd::Normal3f:
                state.normal[0] = cmd.normalData.x;
                state.normal[1] = cmd.normalData.y;
                state.normal[2] = cmd.normalData.z;
                break;
            case DLCmd::LoadIdentity:
                getActiveStack().loadIdentity();
                break;
            case DLCmd::MultMatrix:
            {
                float tmp[16];
                mat4::multiply(tmp, getActiveStack().current(), cmd.matrix);
                mat4::copy(getActiveStack().current(), tmp);
                break;
            }
            case DLCmd::Enable:
                setCapability(cmd.cap, true);
                break;
            case DLCmd::Disable:
                setCapability(cmd.cap, false);
                break;
            case DLCmd::BindTexture:
                state.boundTexture = cmd.textureId;
                break;
            default:
                break;
        }
    }
}

void glCallLists(GLsizei n, GLenum type, const void* lists)
{
    const GLuint* ids = static_cast<const GLuint*>(lists);
    for (GLsizei i = 0; i < n; i++)
        glCallList(ids[i]);
}

// --- Vertex Arrays ---

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    state.vp = {size, type, stride, pointer};
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    state.tp = {size, type, stride, pointer};
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const void* pointer)
{
    state.cp = {size, type, stride, pointer};
}

void glNormalPointer(GLenum type, GLsizei stride, const void* pointer)
{
    state.np = {type, stride, pointer};
}

void glEnableClientState(GLenum array)
{
    setCapability(array, true);
}

void glDisableClientState(GLenum array)
{
    setCapability(array, false);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    if (count <= 0) return;

    ensureInit();

    int stride = state.vp.stride > 0 ? state.vp.stride : 32;
    size_t dataSize = (size_t)(first + count) * stride;

    bool hasTexCoord = state.texCoordArray;
    bool hasColor = state.colorArray;
    bool hasNormal = state.normalArray;

    if (g_recording)
    {
        auto device = Vulkan_Shared::getDevice();

        if (state.boundVBO != 0)
        {
            auto vboIt = g_vbos().find(state.boundVBO);
            if (vboIt == g_vbos().end() || vboIt->second.buffer == VK_NULL_HANDLE) return;

            // Copy data from VBO to immutable buffer for display list (CPU-side, no GPU stall)
            VkBuffer immBuf;
            VkDeviceMemory immMem;
            createBuffer(dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         immBuf, immMem);

            if (vboIt->second.mapped)
            {
                void* mapped;
                vkMapMemory(device, immMem, 0, dataSize, 0, &mapped);
                memcpy(mapped, vboIt->second.mapped, dataSize);
                vkUnmapMemory(device, immMem);
            }

            DLCommand cmd;
            cmd.type = DLCmd::Draw;
            cmd.draw.vbo = immBuf;
            cmd.draw.vboMemory = immMem;
            cmd.draw.mode = mode;
            cmd.draw.first = first;
            cmd.draw.count = count;
            cmd.draw.hasTexture = hasTexCoord;
            cmd.draw.hasColor = hasColor;
            cmd.draw.hasNormal = hasNormal;
            cmd.draw.stride = stride;
            g_recordingList->commands.push_back(cmd);
        }
        else
        {
            const void* basePtr = state.vp.ptr;
            if (!basePtr) return;

            VkBuffer immBuf;
            VkDeviceMemory immMem;
            createBuffer(dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         immBuf, immMem);

            void* mapped;
            vkMapMemory(device, immMem, 0, dataSize, 0, &mapped);
            memcpy(mapped, basePtr, dataSize);
            vkUnmapMemory(device, immMem);

            DLCommand cmd;
            cmd.type = DLCmd::Draw;
            cmd.draw.vbo = immBuf;
            cmd.draw.vboMemory = immMem;
            cmd.draw.mode = mode;
            cmd.draw.first = first;
            cmd.draw.count = count;
            cmd.draw.hasTexture = hasTexCoord;
            cmd.draw.hasColor = hasColor;
            cmd.draw.hasNormal = hasNormal;
            cmd.draw.stride = stride;
            g_recordingList->commands.push_back(cmd);
        }
        return;
    }

    flushState(hasTexCoord, hasColor, hasNormal);

    auto cb = Vulkan_Shared::getCurrentCommandBuffer();

    VkPipeline pipeline = getOrCreatePipeline(mapPrimitive(mode));
    if (pipeline == VK_NULL_HANDLE) return;
    if (pipeline != g_lastBoundPipeline)
    {
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        g_lastBoundPipeline = pipeline;
    }

    if (state.boundVBO != 0)
    {
        auto vboIt = g_vbos().find(state.boundVBO);
        if (vboIt == g_vbos().end() || vboIt->second.buffer == VK_NULL_HANDLE) return;

        if (vboIt->second.isStream)
        {
            // Stream VBO (GL_STREAM_DRAW, Tesselator ring buffer) — copy to per-frame
            // dynamic VB to avoid data races when the ring buffer wraps within a frame.
            const void* vertexData = vboIt->second.mapped;
            if (!vertexData) return;

            int frame = Vulkan_Shared::getCurrentFrame();
            auto& fr = g_frames[frame];

            if (fr.dynamicVBOffset + dataSize > DYNAMIC_VB_SIZE)
            {
                std::cerr << "Vulkan: dynamic VB overflow!" << std::endl;
                return;
            }

            memcpy((uint8_t*)fr.dynamicVBMapped + fr.dynamicVBOffset, vertexData, dataSize);

            VkDeviceSize offset = fr.dynamicVBOffset;
            vkCmdBindVertexBuffers(cb, 0, 1, &fr.dynamicVB, &offset);

            fr.dynamicVBOffset += dataSize;
            fr.dynamicVBOffset = (fr.dynamicVBOffset + 31) & ~31;
        }
        else
        {
            // Static VBO (GL_STATIC_DRAW, chunk meshes) — bind directly.
            // Data is HOST_COHERENT so the GPU sees it immediately after memcpy.
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cb, 0, 1, &vboIt->second.buffer, &offset);
        }
    }
    else
    {
        // Client-side vertex arrays — copy to per-frame dynamic VB
        const void* vertexData = state.vp.ptr;
        if (!vertexData) return;

        int frame = Vulkan_Shared::getCurrentFrame();
        auto& fr = g_frames[frame];

        if (fr.dynamicVBOffset + dataSize > DYNAMIC_VB_SIZE)
        {
            std::cerr << "Vulkan: dynamic VB overflow!" << std::endl;
            return;
        }

        memcpy((uint8_t*)fr.dynamicVBMapped + fr.dynamicVBOffset, vertexData, dataSize);

        VkDeviceSize offset = fr.dynamicVBOffset;
        vkCmdBindVertexBuffers(cb, 0, 1, &fr.dynamicVB, &offset);

        fr.dynamicVBOffset += dataSize;
        // Align to 32 bytes (vertex stride)
        fr.dynamicVBOffset = (fr.dynamicVBOffset + 31) & ~31;
    }

    vkCmdDraw(cb, count, 1, first, 0);
}

// --- VBO ---

void glGenBuffers(GLsizei n, GLuint* buffers)
{
    for (int i = 0; i < n; i++)
    {
        buffers[i] = g_nextVBOId()++;
        g_vbos()[buffers[i]] = VBOData();
    }
}

void glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
    for (int i = 0; i < n; i++)
    {
        auto it = g_vbos().find(buffers[i]);
        if (it != g_vbos().end())
        {
            if (it->second.buffer != VK_NULL_HANDLE)
            {
                // Defer destruction — buffer may still be referenced by in-flight command buffers
                int nextFrame = (Vulkan_Shared::getCurrentFrame() + 1) % MAX_FRAMES;
                g_frames[nextFrame].pendingDeletes.push_back({it->second.buffer, it->second.memory});
            }
            g_vbos().erase(it);
        }
        if (state.boundVBO == buffers[i])
            state.boundVBO = 0;
    }
}

void glBindBuffer(GLenum target, GLuint buffer)
{
    (void)target;
    state.boundVBO = buffer;
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
    (void)target;

    if (state.boundVBO == 0) return;

    auto it = g_vbos().find(state.boundVBO);
    if (it == g_vbos().end()) return;

    auto device = Vulkan_Shared::getDevice();

    // Track usage: GL_STREAM_DRAW VBOs are Tesselator ring buffers (data race risk),
    // GL_STATIC_DRAW VBOs are chunk meshes (safe to bind directly).
    it->second.isStream = (usage == GL_STREAM_DRAW);

    // Recreate buffer if size changed
    if (it->second.buffer != VK_NULL_HANDLE && it->second.size < (size_t)size)
    {
        // Defer destruction — old buffer may still be in use by the other frame's command buffer.
        // Push to the NEXT frame slot; its fence wait guarantees the GPU is done with the old buffer.
        int nextFrame = (Vulkan_Shared::getCurrentFrame() + 1) % MAX_FRAMES;
        g_frames[nextFrame].pendingDeletes.push_back({it->second.buffer, it->second.memory});
        it->second.buffer = VK_NULL_HANDLE;
        it->second.memory = VK_NULL_HANDLE;
        it->second.mapped = nullptr;
    }

    if (it->second.buffer == VK_NULL_HANDLE)
    {
        createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     it->second.buffer, it->second.memory);
        it->second.size = size;

        // Persistently map
        vkMapMemory(device, it->second.memory, 0, size, 0, &it->second.mapped);
    }

    if (data && it->second.mapped)
    {
        memcpy(it->second.mapped, data, size);
    }
}

// ARB variants forward to core
void glGenBuffersARB(GLsizei n, GLuint* buffers) { glGenBuffers(n, buffers); }
void glBindBufferARB(GLenum target, GLuint buffer) { glBindBuffer(target, buffer); }
void glBufferDataARB(GLenum target, GLsizeiptr size, const void* data, GLenum usage) { glBufferData(target, size, data, usage); }

// --- Misc ---

void glColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    state.colorMask[0] = (r != 0);
    state.colorMask[1] = (g != 0);
    state.colorMask[2] = (b != 0);
    state.colorMask[3] = (a != 0);
}

void glCullFace(GLenum mode)
{
    (void)mode;
}

void glLineWidth(GLfloat width)
{
    (void)width;
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
    state.polyOffsetFactor = factor;
    state.polyOffsetUnits = units;
}

void glLogicOp(GLenum opcode)
{
    state.logicOpMode = opcode;
}

void glGetFloatv(GLenum pname, GLfloat* params)
{
    switch (pname)
    {
        case GL_MODELVIEW_MATRIX:
            memcpy(params, mvStack.current(), 64);
            break;
        case GL_PROJECTION_MATRIX:
            memcpy(params, projStack.current(), 64);
            break;
        case GL_CURRENT_COLOR:
            memcpy(params, state.color, 16);
            break;
    }
}

GLenum glGetError()
{
    return GL_NO_ERROR;
}

const GLubyte* glGetString(GLenum name)
{
    switch (name)
    {
        case GL_VENDOR: return (const GLubyte*)"Vulkan Backend";
        case GL_RENDERER: return (const GLubyte*)"Vulkan";
        case GL_VERSION: return (const GLubyte*)"1.0";
        case GL_EXTENSIONS: return (const GLubyte*)"";
        default: return (const GLubyte*)"";
    }
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
{
    (void)x; (void)y; (void)format; (void)type;

    // For screenshots — need to read back swapchain image
    // This requires ending the render pass, copying, and is complex in Vulkan.
    // For now, fill with black (screenshots are non-critical for gameplay).
    memset(pixels, 0, width * height * 3);
}

void glDebugMessageCallback(GLDEBUGPROC callback, const void* userParam)
{
    (void)callback;
    (void)userParam;
}

// --- gluPerspective ---

void gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
    float fovRad = fovy * 3.14159265358979323846f / 180.0f;
    float h = tanf(fovRad / 2.0f);
    float t = zNear * h;
    float b = -t;
    float r = t * aspect;
    float l = -r;

    glFrustum(l, r, b, t, zNear, zFar);
}

// --- Backend queries ---

bool RenderBackend_SupportsVBO()
{
    return true;
}

bool RenderBackend_SupportsARBVBO()
{
    return false;
}
