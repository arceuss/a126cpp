// D3D11 rendering backend
// Implements all gl*() functions declared in Backends/Rendering.h using Direct3D 11.
// Emulates OpenGL 1.x fixed-function pipeline via HLSL shaders and CPU-side state tracking.

#include "Backends/Rendering.h"
#include "Backends/Shared/D3D11.h"
#include "D3D11_Shaders.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <cstring>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

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

// M = M * Translation(x,y,z)
static void translate(float* M, float x, float y, float z)
{
    float T[16];
    identity(T);
    T[12] = x; T[13] = y; T[14] = z;
    float tmp[16];
    multiply(tmp, M, T);
    copy(M, tmp);
}

// M = M * Rotation(angle_deg, ax, ay, az)
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

// M = M * Scale(x,y,z)
static void scale(float* M, float x, float y, float z)
{
    float S[16];
    identity(S);
    S[0] = x; S[5] = y; S[10] = z;
    float tmp[16];
    multiply(tmp, M, S);
    copy(M, tmp);
}

// D3D11 ortho: z maps to [0,1]
static void ortho(float* M, double l, double r, double b, double t, double n, double f)
{
    float O[16];
    memset(O, 0, 64);
    O[0]  = (float)(2.0 / (r - l));
    O[5]  = (float)(2.0 / (t - b));
    O[10] = (float)(-1.0 / (f - n));
    O[12] = (float)(-(r + l) / (r - l));
    O[13] = (float)(-(t + b) / (t - b));
    O[14] = (float)(-n / (f - n));
    O[15] = 1.0f;

    float tmp[16];
    multiply(tmp, M, O);
    copy(M, tmp);
}

// D3D11 frustum: z maps to [0,1]
static void frustum(float* M, double l, double r, double b, double t, double n, double f)
{
    float F[16];
    memset(F, 0, 64);
    F[0]  = (float)(2.0 * n / (r - l));
    F[5]  = (float)(2.0 * n / (t - b));
    F[8]  = (float)((r + l) / (r - l));
    F[9]  = (float)((t + b) / (t - b));
    F[10] = (float)(f / (n - f));
    F[11] = -1.0f;
    F[14] = (float)(n * f / (n - f));

    float tmp[16];
    multiply(tmp, M, F);
    copy(M, tmp);
}

// Transpose column-major to row-major (for HLSL)
static void transpose(float* dst, const float* src)
{
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++)
            dst[r * 4 + c] = src[c * 4 + r];
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
// D3D11 Backend State
// ============================================================================

namespace
{

// Matrix stacks
constexpr int MV_STACK_DEPTH = 32;
constexpr int PROJ_STACK_DEPTH = 8;
constexpr int TEX_STACK_DEPTH = 8;

struct MatrixStack
{
    float matrices[32][16];
    int top = 0;

    MatrixStack()
    {
        // All stacks must start as identity (OpenGL convention).
        // The game only calls glLoadIdentity on modelview/projection,
        // never on the texture matrix.
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

// Render state
struct
{
    // Current vertex attributes
    float color[4] = {1, 1, 1, 1};
    float normal[3] = {0, 0, 1};

    // Enables
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

    // Blend
    GLenum blendSrc = GL_ONE;
    GLenum blendDst = GL_ZERO;

    // Depth
    GLenum depthFunc = GL_LESS;
    double clearDepth = 1.0;

    // Alpha test
    GLenum alphaFunc = GL_ALWAYS;
    float alphaRef = 0.0f;

    // Fog
    GLenum fogMode = GL_EXP;
    float fogDensity = 1.0f;
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    float fogColor[4] = {0, 0, 0, 0};

    // Lighting
    float lightDir[2][4] = {{0, 0, 1, 0}, {0, 0, 1, 0}};
    float lightDiffuse[2][4] = {{1, 1, 1, 1}, {0, 0, 0, 1}};
    float lightAmbient[2][4] = {{0, 0, 0, 1}, {0, 0, 0, 1}};
    float globalAmbient[4] = {0.2f, 0.2f, 0.2f, 1.0f};

    // Color mask
    bool colorMask[4] = {true, true, true, true};

    // Polygon offset
    float polyOffsetFactor = 0.0f;
    float polyOffsetUnits = 0.0f;

    // Logic op
    GLenum logicOpMode = GL_COPY;

    // Clear color
    float clearColor[4] = {0, 0, 0, 0};

    // Viewport
    int viewport[4] = {0, 0, 854, 480};

    // Bound texture
    GLuint boundTexture = 0;

    // Vertex array state
    bool vertexArray = false;
    bool texCoordArray = false;
    bool colorArray = false;
    bool normalArray = false;

    // Vertex pointers
    struct VertexPointer { int size; GLenum type; int stride; const void* ptr; };
    VertexPointer vp = {3, GL_FLOAT, 0, nullptr};
    VertexPointer tp = {2, GL_FLOAT, 0, nullptr};
    VertexPointer cp = {4, GL_UNSIGNED_BYTE, 0, nullptr};
    struct NormalPointer { GLenum type; int stride; const void* ptr; };
    NormalPointer np = {GL_BYTE, 0, nullptr};

    // Bound VBO
    GLuint boundVBO = 0;

    // Shade model
    GLenum shadeModel = GL_SMOOTH;
} state;

// Constant buffer (must be 16-byte aligned, match shader layout)
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

    uint32_t _pad1[3]; // HLSL packing: float4 fogColor must start on 16-byte boundary

    float fogColor[4]; // aligned as float4
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint32_t fogMode;

    float alphaRef;
    uint32_t alphaTestEnabled;

    float _pad2[2];
};
static_assert(sizeof(CBData) % 16 == 0, "CBData must be 16-byte aligned");

// D3D11 resources
ID3D11VertexShader* g_vertexShader = nullptr;
ID3D11PixelShader* g_pixelShader = nullptr;
ID3D11InputLayout* g_inputLayout = nullptr;
ID3D11Buffer* g_constantBuffer = nullptr;
ID3D11Buffer* g_dynamicVB = nullptr;
size_t g_dynamicVBSize = 0;
bool g_initialized = false;

// Texture management
struct TextureData
{
    ID3D11Texture2D* texture = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    int width = 0, height = 0;

    // Per-texture sampler params
    GLenum minFilter = GL_NEAREST;
    GLenum magFilter = GL_NEAREST;
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    bool samplerDirty = true;
    ID3D11SamplerState* sampler = nullptr;
};
// Use function-local statics to avoid static initialization order fiasco.
// Tesselator::instance (global static) calls glGenBuffers() during construction,
// which can happen before namespace-scope maps are initialized.
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
    ID3D11Buffer* buffer = nullptr;
    size_t size = 0;
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
    ID3D11Buffer* vbo;
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

// State object caches
std::unordered_map<uint64_t, ID3D11BlendState*>& g_blendStateCache()
{
    static std::unordered_map<uint64_t, ID3D11BlendState*> inst;
    return inst;
}
std::unordered_map<uint64_t, ID3D11DepthStencilState*>& g_depthStencilCache()
{
    static std::unordered_map<uint64_t, ID3D11DepthStencilState*> inst;
    return inst;
}
std::unordered_map<uint32_t, ID3D11RasterizerState*>& g_rasterizerCache()
{
    static std::unordered_map<uint32_t, ID3D11RasterizerState*> inst;
    return inst;
}
std::unordered_map<uint32_t, ID3D11SamplerState*>& g_samplerCache()
{
    static std::unordered_map<uint32_t, ID3D11SamplerState*> inst;
    return inst;
}

// Pixel store state
int g_packAlignment = 4;
int g_unpackAlignment = 4;

// D3D11 blend factor mapping
D3D11_BLEND mapBlendFactor(GLenum gl)
{
    switch (gl)
    {
        case GL_ZERO: return D3D11_BLEND_ZERO;
        case GL_ONE: return D3D11_BLEND_ONE;
        case GL_SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
        case GL_ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
        case GL_DST_COLOR: return D3D11_BLEND_DEST_COLOR;
        case GL_SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
        default: return D3D11_BLEND_ONE;
    }
}

D3D11_COMPARISON_FUNC mapDepthFunc(GLenum gl)
{
    switch (gl)
    {
        case GL_NEVER: return D3D11_COMPARISON_NEVER;
        case GL_LESS: return D3D11_COMPARISON_LESS;
        case GL_EQUAL: return D3D11_COMPARISON_EQUAL;
        case GL_LEQUAL: return D3D11_COMPARISON_LESS_EQUAL;
        case GL_GREATER: return D3D11_COMPARISON_GREATER;
        case GL_ALWAYS: return D3D11_COMPARISON_ALWAYS;
        default: return D3D11_COMPARISON_LESS;
    }
}

void initShaders()
{
    auto device = D3D11_Shared::getDevice();
    if (!device) return;

    // Compile vertex shader
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    HRESULT hr = D3DCompile(g_vsSource, strlen(g_vsSource), "VS", nullptr, nullptr,
                            "main", "vs_4_0", 0, 0, &vsBlob, &errBlob);
    if (FAILED(hr))
    {
        if (errBlob)
        {
            std::cerr << "VS compile error: " << (char*)errBlob->GetBufferPointer() << std::endl;
            errBlob->Release();
        }
        return;
    }

    hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_vertexShader);
    if (FAILED(hr)) { vsBlob->Release(); return; }

    // Input layout (32-byte Tesselator vertex format)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,      0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R8G8B8A8_SNORM,      0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr = device->CreateInputLayout(layout, 4, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_inputLayout);
    vsBlob->Release();
    if (FAILED(hr)) return;

    // Compile pixel shader
    ID3DBlob* psBlob = nullptr;
    hr = D3DCompile(g_psSource, strlen(g_psSource), "PS", nullptr, nullptr,
                    "main", "ps_4_0", 0, 0, &psBlob, &errBlob);
    if (FAILED(hr))
    {
        if (errBlob)
        {
            std::cerr << "PS compile error: " << (char*)errBlob->GetBufferPointer() << std::endl;
            errBlob->Release();
        }
        return;
    }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pixelShader);
    psBlob->Release();
    if (FAILED(hr)) return;

    // Constant buffer
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CBData);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cbDesc, nullptr, &g_constantBuffer);

    g_initialized = true;
}

void ensureInit()
{
    if (!g_initialized)
        initShaders();
}

void ensureDynamicVB(size_t needed)
{
    if (g_dynamicVB && g_dynamicVBSize >= needed)
        return;

    if (g_dynamicVB)
        g_dynamicVB->Release();

    g_dynamicVBSize = (std::max)(needed, (size_t)(1024 * 1024)); // at least 1MB

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = (UINT)g_dynamicVBSize;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    D3D11_Shared::getDevice()->CreateBuffer(&desc, nullptr, &g_dynamicVB);
}

ID3D11BlendState* getBlendState()
{
    uint64_t key = 0;
    key |= (uint64_t)state.blend;
    key |= (uint64_t)state.blendSrc << 1;
    key |= (uint64_t)state.blendDst << 8;
    key |= (uint64_t)state.colorMask[0] << 16;
    key |= (uint64_t)state.colorMask[1] << 17;
    key |= (uint64_t)state.colorMask[2] << 18;
    key |= (uint64_t)state.colorMask[3] << 19;
    key |= (uint64_t)state.logicOp << 20;

    auto it = g_blendStateCache().find(key);
    if (it != g_blendStateCache().end())
        return it->second;

    auto device = D3D11_Shared::getDevice();

    // Try D3D11.1 for logic ops
    if (state.logicOp)
    {
        ID3D11Device1* device1 = D3D11_Shared::getDevice1();
        if (device1)
        {
            D3D11_BLEND_DESC1 desc = {};
            desc.RenderTarget[0].LogicOpEnable = TRUE;
            desc.RenderTarget[0].LogicOp = D3D11_LOGIC_OP_OR_REVERSE;
            desc.RenderTarget[0].RenderTargetWriteMask = 0;
            if (state.colorMask[0]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
            if (state.colorMask[1]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
            if (state.colorMask[2]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
            if (state.colorMask[3]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

            ID3D11BlendState1* bs1 = nullptr;
            device1->CreateBlendState1(&desc, &bs1);
            g_blendStateCache()[key] = bs1;
            return bs1;
        }
    }

    D3D11_BLEND_DESC desc = {};
    desc.RenderTarget[0].BlendEnable = state.blend ? TRUE : FALSE;
    desc.RenderTarget[0].SrcBlend = mapBlendFactor(state.blendSrc);
    desc.RenderTarget[0].DestBlend = mapBlendFactor(state.blendDst);
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = 0;
    if (state.colorMask[0]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
    if (state.colorMask[1]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    if (state.colorMask[2]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    if (state.colorMask[3]) desc.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

    ID3D11BlendState* bs = nullptr;
    device->CreateBlendState(&desc, &bs);
    g_blendStateCache()[key] = bs;
    return bs;
}

ID3D11DepthStencilState* getDepthStencilState()
{
    uint64_t key = 0;
    key |= (uint64_t)state.depthTest;
    key |= (uint64_t)state.depthWrite << 1;
    key |= (uint64_t)state.depthFunc << 4;

    auto it = g_depthStencilCache().find(key);
    if (it != g_depthStencilCache().end())
        return it->second;

    D3D11_DEPTH_STENCIL_DESC desc = {};
    desc.DepthEnable = state.depthTest ? TRUE : FALSE;
    desc.DepthWriteMask = state.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    desc.DepthFunc = mapDepthFunc(state.depthFunc);
    desc.StencilEnable = FALSE;

    ID3D11DepthStencilState* dss = nullptr;
    D3D11_Shared::getDevice()->CreateDepthStencilState(&desc, &dss);
    g_depthStencilCache()[key] = dss;
    return dss;
}

ID3D11RasterizerState* getRasterizerState()
{
    uint32_t key = 0;
    key |= (uint32_t)state.cullFace;
    key |= (uint32_t)state.polyOffsetFill << 1;
    // Encode offset values roughly
    key |= ((uint32_t)(state.polyOffsetFactor * 100) & 0xFFF) << 4;
    key |= ((uint32_t)(state.polyOffsetUnits * 100) & 0xFFF) << 16;

    auto it = g_rasterizerCache().find(key);
    if (it != g_rasterizerCache().end())
        return it->second;

    D3D11_RASTERIZER_DESC desc = {};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = state.cullFace ? D3D11_CULL_BACK : D3D11_CULL_NONE;
    desc.FrontCounterClockwise = TRUE; // OpenGL convention: CCW = front face
    desc.DepthClipEnable = TRUE;
    desc.ScissorEnable = FALSE;
    desc.MultisampleEnable = FALSE;
    desc.AntialiasedLineEnable = FALSE;

    if (state.polyOffsetFill)
    {
        // SlopeScaledDepthBias maps 1:1 to GL's factor parameter.
        // DepthBias is in minimum-representable-depth-value units;
        // GL's units param is already in that space, just cast to int.
        desc.SlopeScaledDepthBias = state.polyOffsetFactor;
        desc.DepthBias = (int)state.polyOffsetUnits;
    }

    ID3D11RasterizerState* rs = nullptr;
    D3D11_Shared::getDevice()->CreateRasterizerState(&desc, &rs);
    g_rasterizerCache()[key] = rs;
    return rs;
}

ID3D11SamplerState* getSamplerForTexture(TextureData& tex)
{
    if (!tex.samplerDirty && tex.sampler)
        return tex.sampler;

    uint32_t key = 0;
    key |= (uint32_t)tex.minFilter;
    key |= (uint32_t)tex.magFilter << 16;
    key |= (uint32_t)(tex.wrapS == GL_CLAMP ? 1 : 0) << 8;
    key |= (uint32_t)(tex.wrapT == GL_CLAMP ? 1 : 0) << 9;

    auto it = g_samplerCache().find(key);
    if (it != g_samplerCache().end())
    {
        tex.sampler = it->second;
        tex.samplerDirty = false;
        return tex.sampler;
    }

    D3D11_SAMPLER_DESC desc = {};

    bool minNearest = (tex.minFilter == GL_NEAREST || tex.minFilter == GL_NEAREST_MIPMAP_NEAREST);
    bool magNearest = (tex.magFilter == GL_NEAREST);

    if (minNearest && magNearest)
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    else if (!minNearest && !magNearest)
        desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    else if (minNearest)
        desc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
    else
        desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;

    desc.AddressU = (tex.wrapS == GL_CLAMP) ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = (tex.wrapT == GL_CLAMP) ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MaxLOD = D3D11_FLOAT32_MAX;

    ID3D11SamplerState* ss = nullptr;
    D3D11_Shared::getDevice()->CreateSamplerState(&desc, &ss);
    g_samplerCache()[key] = ss;
    tex.sampler = ss;
    tex.samplerDirty = false;
    return ss;
}

void flushState(bool hasTexCoord, bool hasColor, bool hasNormal)
{
    ensureInit();
    auto ctx = D3D11_Shared::getContext();
    if (!ctx) return;

    // Update constant buffer
    CBData cb = {};

    // MVP = Projection * ModelView
    float mvp[16];
    mat4::multiply(mvp, projStack.current(), mvStack.current());
    mat4::transpose(cb.mvp, mvp);
    mat4::transpose(cb.mv, mvStack.current());
    mat4::transpose(cb.texMat, texStack.current());

    // Lighting
    cb.lightingEnabled = state.lighting ? 1 : 0;
    memcpy(cb.lightDir0, state.lightDir[0], 16);
    cb.lightDir0[3] = state.light0 ? 1.0f : 0.0f;
    memcpy(cb.lightDir1, state.lightDir[1], 16);
    cb.lightDir1[3] = state.light1 ? 1.0f : 0.0f;
    memcpy(cb.lightDiffuse0, state.lightDiffuse[0], 16);
    memcpy(cb.lightDiffuse1, state.lightDiffuse[1], 16);
    memcpy(cb.globalAmbient, state.globalAmbient, 16);

    memcpy(cb.currentColor, state.color, 16);
    cb.currentNormal[0] = state.normal[0];
    cb.currentNormal[1] = state.normal[1];
    cb.currentNormal[2] = state.normal[2];
    cb.currentNormal[3] = 0.0f;

    cb.textureEnabled = (state.texture2D && state.boundTexture != 0) ? 1 : 0;
    cb.hasVertexColor = hasColor ? 1 : 0;
    cb.hasVertexNormal = hasNormal ? 1 : 0;
    cb.hasVertexTexCoord = hasTexCoord ? 1 : 0;

    // Fog
    memcpy(cb.fogColor, state.fogColor, 16);
    cb.fogStart = state.fogStart;
    cb.fogEnd = state.fogEnd;
    cb.fogDensity = state.fogDensity;
    if (!state.fog)
        cb.fogMode = 0;
    else if (state.fogMode == GL_LINEAR)
        cb.fogMode = 1;
    else
        cb.fogMode = 2; // EXP

    cb.alphaRef = state.alphaRef;
    cb.alphaTestEnabled = state.alphaTest ? 1 : 0;

    D3D11_MAPPED_SUBRESOURCE mapped;
    ctx->Map(g_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &cb, sizeof(cb));
    ctx->Unmap(g_constantBuffer, 0);

    // Set pipeline state
    ctx->IASetInputLayout(g_inputLayout);
    ctx->VSSetShader(g_vertexShader, nullptr, 0);
    ctx->PSSetShader(g_pixelShader, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &g_constantBuffer);
    ctx->PSSetConstantBuffers(0, 1, &g_constantBuffer);

    // Bind texture + sampler
    if (state.texture2D && state.boundTexture != 0)
    {
        auto it = g_textures().find(state.boundTexture);
        if (it != g_textures().end() && it->second.srv)
        {
            ctx->PSSetShaderResources(0, 1, &it->second.srv);
            auto sampler = getSamplerForTexture(it->second);
            ctx->PSSetSamplers(0, 1, &sampler);
        }
    }

    // Blend state
    float blendFactor[4] = {1, 1, 1, 1};
    ctx->OMSetBlendState(getBlendState(), blendFactor, 0xFFFFFFFF);

    // Depth stencil
    ctx->OMSetDepthStencilState(getDepthStencilState(), 0);

    // Rasterizer
    ctx->RSSetState(getRasterizerState());

    // Viewport — convert from GL's bottom-left origin to D3D11's top-left origin
    D3D11_VIEWPORT vp;
    vp.TopLeftX = (float)state.viewport[0];
    int bbHeight = D3D11_Shared::getBackbufferHeight();
    vp.TopLeftY = (float)(bbHeight - state.viewport[1] - state.viewport[3]);
    vp.Width = (float)state.viewport[2];
    vp.Height = (float)state.viewport[3];
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ctx->RSSetViewports(1, &vp);

    // Render targets
    auto rtv = D3D11_Shared::getRenderTargetView();
    auto dsv = D3D11_Shared::getDepthStencilView();
    ctx->OMSetRenderTargets(1, &rtv, dsv);
}

D3D_PRIMITIVE_TOPOLOGY mapPrimitive(GLenum mode)
{
    switch (mode)
    {
        case GL_TRIANGLES: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case GL_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case GL_TRIANGLE_FAN: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; // approximate
        case GL_LINES: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case GL_LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case GL_QUADS: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; // quads pre-converted by Tesselator
        default: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
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
            // OpenGL transforms light position by current modelview matrix.
            // This puts the light direction into eye space -- matching OpenGL convention
            // where vertex normals are also transformed to eye space during rendering.
            float transformed[4];
            mat4::transformVec4(transformed, mvStack.current(), params);
            // Normalize direction for directional lights (w=0)
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
            // Ignored (game sets to 0)
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
    // Always GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE in this game
    // The shader handles this implicitly when colorMaterial is enabled
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
    auto ctx = D3D11_Shared::getContext();
    if (!ctx) return;

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        auto rtv = D3D11_Shared::getRenderTargetView();
        if (rtv) ctx->ClearRenderTargetView(rtv, state.clearColor);
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        auto dsv = D3D11_Shared::getDepthStencilView();
        if (dsv) ctx->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH, (float)state.clearDepth, 0);
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
    for (int i = 0; i < n; i++)
    {
        auto it = g_textures().find(textures[i]);
        if (it != g_textures().end())
        {
            if (it->second.srv) it->second.srv->Release();
            if (it->second.texture) it->second.texture->Release();
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

    auto device = D3D11_Shared::getDevice();
    if (!device) return;

    // Release old
    if (it->second.srv) { it->second.srv->Release(); it->second.srv = nullptr; }
    if (it->second.texture) { it->second.texture->Release(); it->second.texture = nullptr; }

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels;
    initData.SysMemPitch = width * 4;

    HRESULT hr = device->CreateTexture2D(&desc, pixels ? &initData : nullptr, &it->second.texture);
    if (FAILED(hr)) return;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(it->second.texture, &srvDesc, &it->second.srv);

    it->second.width = width;
    it->second.height = height;
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
    (void)target; (void)level; (void)format; (void)type;

    auto it = g_textures().find(state.boundTexture);
    if (it == g_textures().end() || !it->second.texture) return;

    D3D11_BOX box;
    box.left = xoffset;
    box.top = yoffset;
    box.right = xoffset + width;
    box.bottom = yoffset + height;
    box.front = 0;
    box.back = 1;

    D3D11_Shared::getContext()->UpdateSubresource(it->second.texture, 0, &box, pixels, width * 4, 0);
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
    for (GLsizei i = 0; i < range; i++)
    {
        auto it = g_displayLists().find(list + i);
        if (it != g_displayLists().end())
        {
            // Release GPU buffers in draw commands
            for (auto& cmd : it->second.commands)
            {
                if (cmd.type == DLCmd::Draw && cmd.draw.vbo)
                    cmd.draw.vbo->Release();
            }
            g_displayLists().erase(it);
        }
    }
}

void glNewList(GLuint list, GLenum mode)
{
    (void)mode; // Always GL_COMPILE
    g_recording = true;
    g_recordingListId = list;
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
                auto ctx = D3D11_Shared::getContext();
                if (!ctx || !cmd.draw.vbo) break;

                flushState(cmd.draw.hasTexture, cmd.draw.hasColor, cmd.draw.hasNormal);

                UINT stride = cmd.draw.stride;
                UINT offset = 0;
                ctx->IASetVertexBuffers(0, 1, &cmd.draw.vbo, &stride, &offset);
                ctx->IASetPrimitiveTopology(mapPrimitive(cmd.draw.mode));
                ctx->Draw(cmd.draw.count, cmd.draw.first);
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
    auto ctx = D3D11_Shared::getContext();
    auto device = D3D11_Shared::getDevice();
    if (!ctx || !device) return;

    int stride = state.vp.stride > 0 ? state.vp.stride : 32;
    size_t dataSize = (size_t)(first + count) * stride;

    bool hasTexCoord = state.texCoordArray;
    bool hasColor = state.colorArray;
    bool hasNormal = state.normalArray;

    if (g_recording)
    {
        // Record into display list — copy vertex data to immutable buffer
        const void* basePtr;
        if (state.boundVBO != 0)
        {
            // VBO is bound — data was already uploaded via glBufferData
            // We need to retrieve it... For display lists recorded with VBOs,
            // we stored the data in the VBO. Copy from the VBO.
            auto vboIt = g_vbos().find(state.boundVBO);
            if (vboIt == g_vbos().end() || !vboIt->second.buffer) return;

            // Create a staging buffer to read back the VBO data
            D3D11_BUFFER_DESC stagingDesc = {};
            stagingDesc.ByteWidth = (UINT)vboIt->second.size;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

            ID3D11Buffer* staging = nullptr;
            device->CreateBuffer(&stagingDesc, nullptr, &staging);
            ctx->CopyResource(staging, vboIt->second.buffer);

            D3D11_MAPPED_SUBRESOURCE mapped;
            ctx->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);

            // Create immutable buffer from the data
            D3D11_BUFFER_DESC immDesc = {};
            immDesc.ByteWidth = (UINT)dataSize;
            immDesc.Usage = D3D11_USAGE_IMMUTABLE;
            immDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = mapped.pData;

            ID3D11Buffer* immBuf = nullptr;
            device->CreateBuffer(&immDesc, &initData, &immBuf);

            ctx->Unmap(staging, 0);
            staging->Release();

            DLCommand cmd;
            cmd.type = DLCmd::Draw;
            cmd.draw.vbo = immBuf;
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
            // Client-side vertex arrays
            basePtr = state.vp.ptr;
            if (!basePtr) return;

            // The vertex pointer points to offset 0 of the vertex data
            // (Tesselator uses offset 0 for position, 12 for texcoord, etc.)
            // We need to go back to the start of the interleaved buffer
            const char* bufStart = static_cast<const char*>(basePtr);

            D3D11_BUFFER_DESC immDesc = {};
            immDesc.ByteWidth = (UINT)dataSize;
            immDesc.Usage = D3D11_USAGE_IMMUTABLE;
            immDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            D3D11_SUBRESOURCE_DATA initData = {};
            initData.pSysMem = bufStart;

            ID3D11Buffer* immBuf = nullptr;
            device->CreateBuffer(&immDesc, &initData, &immBuf);

            DLCommand cmd;
            cmd.type = DLCmd::Draw;
            cmd.draw.vbo = immBuf;
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

    if (state.boundVBO != 0)
    {
        auto vboIt = g_vbos().find(state.boundVBO);
        if (vboIt != g_vbos().end() && vboIt->second.buffer)
        {
            UINT uStride = stride;
            UINT uOffset = 0;
            ctx->IASetVertexBuffers(0, 1, &vboIt->second.buffer, &uStride, &uOffset);
        }
    }
    else
    {
        // Upload client-side data to dynamic VB
        const void* vertexData = state.vp.ptr;
        if (!vertexData) return;

        ensureDynamicVB(dataSize);

        D3D11_MAPPED_SUBRESOURCE mapped;
        ctx->Map(g_dynamicVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, vertexData, dataSize);
        ctx->Unmap(g_dynamicVB, 0);

        UINT uStride = stride;
        UINT uOffset = 0;
        ctx->IASetVertexBuffers(0, 1, &g_dynamicVB, &uStride, &uOffset);
    }

    ctx->IASetPrimitiveTopology(mapPrimitive(mode));
    ctx->Draw(count, first);
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

void glBindBuffer(GLenum target, GLuint buffer)
{
    (void)target;
    state.boundVBO = buffer;
}

void glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
    (void)target; (void)usage;

    if (state.boundVBO == 0) return;

    auto it = g_vbos().find(state.boundVBO);
    if (it == g_vbos().end()) return;

    auto device = D3D11_Shared::getDevice();
    auto ctx = D3D11_Shared::getContext();
    if (!device || !ctx) return;

    // Recreate buffer if size changed
    if (it->second.buffer && it->second.size < (size_t)size)
    {
        it->second.buffer->Release();
        it->second.buffer = nullptr;
    }

    if (!it->second.buffer)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = (UINT)size;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&desc, nullptr, &it->second.buffer);
        it->second.size = size;
    }

    if (data && it->second.buffer)
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        ctx->Map(it->second.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, data, size);
        ctx->Unmap(it->second.buffer, 0);
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
    (void)mode; // Always GL_BACK in this game
}

void glLineWidth(GLfloat width)
{
    (void)width; // D3D11 doesn't support line width
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
        case GL_VENDOR: return (const GLubyte*)"D3D11 Backend";
        case GL_RENDERER: return (const GLubyte*)"Direct3D 11";
        case GL_VERSION: return (const GLubyte*)"1.0";
        case GL_EXTENSIONS: return (const GLubyte*)"";
        default: return (const GLubyte*)"";
    }
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
{
    (void)x; (void)y; (void)format; (void)type;

    auto ctx = D3D11_Shared::getContext();
    auto swapChain = D3D11_Shared::getSwapChain();
    auto device = D3D11_Shared::getDevice();
    if (!ctx || !swapChain || !device) return;

    // Get back buffer
    ID3D11Texture2D* backBuffer = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (!backBuffer) return;

    D3D11_TEXTURE2D_DESC bbDesc;
    backBuffer->GetDesc(&bbDesc);

    // Create staging texture
    D3D11_TEXTURE2D_DESC stagingDesc = bbDesc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ID3D11Texture2D* staging = nullptr;
    device->CreateTexture2D(&stagingDesc, nullptr, &staging);
    if (!staging) { backBuffer->Release(); return; }

    ctx->CopyResource(staging, backBuffer);

    D3D11_MAPPED_SUBRESOURCE mapped;
    ctx->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);

    // Convert RGBA (D3D11) to BGR (GL_BGR_EXT) and flip vertically (D3D11 origin is top-left, GL is bottom-left)
    unsigned char* dst = static_cast<unsigned char*>(pixels);
    unsigned char* src = static_cast<unsigned char*>(mapped.pData);

    for (int row = 0; row < height; row++)
    {
        int srcRow = (height - 1 - row); // flip
        unsigned char* srcLine = src + srcRow * mapped.RowPitch;
        unsigned char* dstLine = dst + row * width * 3;

        for (int col = 0; col < width; col++)
        {
            dstLine[col * 3 + 0] = srcLine[col * 4 + 2]; // B
            dstLine[col * 3 + 1] = srcLine[col * 4 + 1]; // G
            dstLine[col * 3 + 2] = srcLine[col * 4 + 0]; // R
        }
    }

    ctx->Unmap(staging, 0);
    staging->Release();
    backBuffer->Release();
}

void glDebugMessageCallback(GLDEBUGPROC callback, const void* userParam)
{
    (void)callback;
    (void)userParam;
    // No-op for D3D11 backend (could wire to D3D11 debug layer)
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
    return true; // D3D11 always supports buffer objects
}

bool RenderBackend_SupportsARBVBO()
{
    return false; // Use core path
}
