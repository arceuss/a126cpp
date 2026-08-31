#define NOMINMAX
#include <windows.h>

#include "backends/Backend.h"
#include "backends/D3D12/Shaders.h"

#include <algorithm>
#include <array>
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
#include <chrono>
#endif
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <cstring>
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
#include <fstream>
#include <iomanip>
#endif
#include <iostream>
#include <limits>
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
#include <locale>
#endif
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "backends/Platform/Platform.h"
#include "legacygl/LegacyGL.h"
#include "legacygl/PixelFormat.h"
#include "legacygl/Sink.h"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace d3d12backend
{

using Microsoft::WRL::ComPtr;

static const int D3D12_TEXTURE_LEVELS = 16;
static const int D3D12_FRAMES_IN_FLIGHT = 2;
static const UINT64 D3D12_UPLOAD_CHUNK_SIZE = 4ull * 1024ull * 1024ull;
static const UINT64 D3D12_RESIDENT_PAGE_SIZE = 16ull * 1024ull * 1024ull;
static const UINT D3D12_SRV_DESCRIPTOR_COUNT = 16384;
static const UINT D3D12_FIRST_TEXTURE_SRV_DESCRIPTOR = 2;
static const UINT D3D12_SAMPLER_DESCRIPTOR_COUNT = 128;
static const DXGI_FORMAT D3D12_COLOR_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT D3D12_COLOR_RESOURCE_FORMAT = DXGI_FORMAT_R8G8B8A8_TYPELESS;
static const DXGI_FORMAT D3D12_LOGIC_COLOR_FORMAT = DXGI_FORMAT_R8G8B8A8_UINT;
static const DXGI_FORMAT D3D12_DEPTH_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;

struct D3D12GPUVertex
{
	float position[3];
	float color[4];
	float normal[3];
	float texCoord[2];
	float flatPosition[3];
	float flatColor[4];
	float flatNormal[3];
};

struct alignas(16) D3D12GPUMaterial
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float emission[4];
	float shininess[4];
};

struct alignas(16) D3D12GPULight
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float positionEye[4];
	float spotDirectionCutoff[4];
	float attenuationExponent[4];
};

struct alignas(16) D3D12GPUState
{
	float modelView[16];
	float projection[16];
	float texture[16];
	float normal[16];
	float globalAmbient[4];
	D3D12GPUMaterial frontMaterial;
	D3D12GPUMaterial backMaterial;
	D3D12GPULight lights[8];
	float fogColor[4];
	float fogParams[4];
	float textureSize[4];
	float normalParams[4];
	unsigned int flags0[4];
	unsigned int flags1[4];
	unsigned int flags2[4];
	unsigned int flags3[4];
	float currentColor[4];
	float currentNormal[4];
	float currentTexCoord[4];
	unsigned int flags4[4];
};

static_assert(sizeof(D3D12GPUVertex) == 88, "D3D12 vertex ABI changed");
static_assert(sizeof(unsigned int) == 4, "D3D12 shader flags require 32-bit unsigned int");
static_assert(sizeof(D3D12GPUMaterial) == 80, "D3D12 shader material ABI changed");
static_assert(sizeof(D3D12GPULight) == 96, "D3D12 shader light ABI changed");
static_assert(offsetof(D3D12GPUState, modelView) == 0, "D3D12 shader model-view ABI changed");
static_assert(offsetof(D3D12GPUState, projection) == 64, "D3D12 shader projection ABI changed");
static_assert(offsetof(D3D12GPUState, texture) == 128, "D3D12 shader texture matrix ABI changed");
static_assert(offsetof(D3D12GPUState, normal) == 192, "D3D12 shader normal matrix ABI changed");
static_assert(offsetof(D3D12GPUState, globalAmbient) == 256, "D3D12 shader ambient ABI changed");
static_assert(offsetof(D3D12GPUState, frontMaterial) == 272, "D3D12 shader front material ABI changed");
static_assert(offsetof(D3D12GPUState, backMaterial) == 352, "D3D12 shader back material ABI changed");
static_assert(offsetof(D3D12GPUState, lights) == 432, "D3D12 shader light ABI changed");
static_assert(offsetof(D3D12GPUState, fogColor) == 1200, "D3D12 shader fog color ABI changed");
static_assert(offsetof(D3D12GPUState, fogParams) == 1216, "D3D12 shader fog parameters ABI changed");
static_assert(offsetof(D3D12GPUState, textureSize) == 1232, "D3D12 shader texture size ABI changed");
static_assert(offsetof(D3D12GPUState, normalParams) == 1248, "D3D12 shader normal parameters ABI changed");
static_assert(offsetof(D3D12GPUState, flags0) == 1264, "D3D12 shader flags0 ABI changed");
static_assert(offsetof(D3D12GPUState, flags1) == 1280, "D3D12 shader flags1 ABI changed");
static_assert(offsetof(D3D12GPUState, flags2) == 1296, "D3D12 shader flags2 ABI changed");
static_assert(offsetof(D3D12GPUState, flags3) == 1312, "D3D12 shader flags3 ABI changed");
static_assert(offsetof(D3D12GPUState, currentColor) == 1328, "D3D12 shader current color ABI changed");
static_assert(offsetof(D3D12GPUState, currentNormal) == 1344, "D3D12 shader current normal ABI changed");
static_assert(offsetof(D3D12GPUState, currentTexCoord) == 1360, "D3D12 shader current texture coordinate ABI changed");
static_assert(offsetof(D3D12GPUState, flags4) == 1376, "D3D12 shader flags4 ABI changed");
static_assert(sizeof(D3D12GPUState) == 1392, "D3D12 shader block ABI changed");

struct UploadChunk
{
	ComPtr<ID3D12Resource> resource;
	UINT64 size = 0;
	UINT64 used = 0;
	unsigned char *mapped = nullptr;
};

struct UploadAllocation
{
	ID3D12Resource *resource = nullptr;
	UINT64 offset = 0;
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = 0;
	void *mapped = nullptr;
};

struct ResidentAllocation;

struct FrameResources
{
	ComPtr<ID3D12CommandAllocator> allocator;
	std::vector<UploadChunk> uploadChunks;
	std::vector<ComPtr<ID3D12Resource>> retainedResources;
	std::unordered_set<ID3D12Resource *> retainedResourceSet;
	std::vector<std::shared_ptr<ResidentAllocation>> residentAllocations;
	std::unordered_set<ResidentAllocation *> retainedAllocationSet;
	std::vector<UINT> retiredSrvDescriptors;
	UINT64 fenceValue = 0;
};

struct TextureLevel
{
	int width = 0;
	int height = 0;
	bool defined = false;
	std::vector<unsigned char> rgba;
};

struct TextureStorage
{
	ComPtr<ID3D12Resource> resource;
	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	UINT srvIndex = 0;
	int width = 0;
	int height = 0;
};

struct D3D12Texture
{
	std::array<TextureLevel, D3D12_TEXTURE_LEVELS> levels;
	TextureStorage storage;
	unsigned int derivedWrapS = GL_REPEAT;
	unsigned int derivedWrapT = GL_REPEAT;
	unsigned char derivedBorder[4] = { 0, 0, 0, 0 };
	bool derivedHasGutter = false;
	bool derivedDirty = true;
};

struct TextureBinding
{
	D3D12_GPU_DESCRIPTOR_HANDLE srv = {};
	D3D12_GPU_DESCRIPTOR_HANDLE sampler = {};
	ComPtr<ID3D12Resource> resource;
};

struct SamplerKey
{
	unsigned int minFilter = 0;
	unsigned int magFilter = 0;
	unsigned int wrapS = 0;
	unsigned int wrapT = 0;

	bool operator<(const SamplerKey &other) const
	{
		return std::tie(minFilter, magFilter, wrapS, wrapT) <
			std::tie(other.minFilter, other.magFilter, other.wrapS, other.wrapT);
	}

	bool operator==(const SamplerKey &other) const
	{
		return !(*this < other) && !(other < *this);
	}
};

struct ResidentFreeRange
{
	UINT64 offset = 0;
	UINT64 size = 0;
};

struct ResidentPage
{
	ComPtr<ID3D12Resource> resource;
	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	std::vector<ResidentFreeRange> freeRanges;
};

struct ResidentAllocation
{
	ResidentPage *page = nullptr;
	UINT64 offset = 0;
	UINT64 size = 0;
};

struct ResidentGeometryEntry
{
	std::shared_ptr<ResidentAllocation> allocation;
	legacygl::Topology topology = legacygl::Topology::Triangles;
	UINT vertexCount = 0;
	bool hasColor = false;
	bool hasNormal = false;
	bool hasTexCoord = false;
};


struct PipelineKey
{
	legacygl::Topology topology = legacygl::Topology::Triangles;
	bool depthTest = false;
	bool depthWrite = false;
	unsigned int depthFunction = 0;
	bool cullFace = false;
	unsigned int cullFaceMode = 0;
	unsigned int frontFaceMode = 0;
	bool blend = false;
	unsigned int blendSource = 0;
	unsigned int blendDestination = 0;
	bool logicOp = false;
	unsigned int logicOpcode = 0;
	unsigned int colorWriteMask = 0;
	bool stencilTest = false;
	bool depthBias = false;
	std::uint32_t polygonOffsetFactor = 0;
	std::uint32_t polygonOffsetUnits = 0;

	bool operator<(const PipelineKey &other) const
	{
		return std::tie(topology, depthTest, depthWrite, depthFunction, cullFace,
			cullFaceMode, frontFaceMode, blend, blendSource, blendDestination,
			logicOp, logicOpcode, colorWriteMask, stencilTest, depthBias,
			polygonOffsetFactor, polygonOffsetUnits) <
			std::tie(other.topology, other.depthTest, other.depthWrite, other.depthFunction,
			other.cullFace, other.cullFaceMode, other.frontFaceMode, other.blend,
			other.blendSource, other.blendDestination, other.logicOp, other.logicOpcode,
			other.colorWriteMask, other.stencilTest, other.depthBias,
			other.polygonOffsetFactor, other.polygonOffsetUnits);
	}

	bool operator==(const PipelineKey &other) const
	{
		return !(*this < other) && !(other < *this);
	}
};

struct CommandListState
{
	PipelineKey legacyPipelineKey;
	ID3D12RootSignature *rootSignature = nullptr;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	D3D12_VIEWPORT viewport = {};
	D3D12_RECT scissor = {};
	D3D12_GPU_DESCRIPTOR_HANDLE samplerTable = {};
	bool legacyPipelineValid = false;
	bool rootSignatureValid = false;
	bool topologyValid = false;
	bool viewportValid = false;
	bool scissorValid = false;
	bool samplerTableValid = false;
	bool legacyDescriptorHeapsValid = false;
};

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
enum class RenderTargetPassEmulation
{
	Offscreen,
	PartialClear,
	Present
};

struct Diagnostics
{
	std::uint64_t legacyPipelineRequests = 0;
	std::uint64_t legacyPipelineCurrentHits = 0;
	std::uint64_t legacyPipelineMapLookups = 0;
	std::uint64_t legacyPipelineMapHits = 0;
	std::uint64_t legacyPipelineCreates = 0;
	std::uint64_t legacyPipelineCreationNanoseconds = 0;
	std::uint64_t legacyPipelineActualBinds = 0;
	std::uint64_t rootSignatureRequests = 0;
	std::uint64_t rootSignatureCurrentHits = 0;
	std::uint64_t rootSignatureActualBinds = 0;
	std::uint64_t topologyRequests = 0;
	std::uint64_t topologyCurrentHits = 0;
	std::uint64_t topologyActualBinds = 0;
	std::uint64_t viewportRequests = 0;
	std::uint64_t viewportCurrentHits = 0;
	std::uint64_t viewportActualSets = 0;
	std::uint64_t scissorRequests = 0;
	std::uint64_t scissorCurrentHits = 0;
	std::uint64_t scissorActualSets = 0;
	std::uint64_t samplerRequests = 0;
	std::uint64_t samplerCurrentHits = 0;
	std::uint64_t samplerMapLookups = 0;
	std::uint64_t samplerMapHits = 0;
	std::uint64_t samplerAllocations = 0;
	std::uint64_t samplerTableRequests = 0;
	std::uint64_t samplerTableCurrentHits = 0;
	std::uint64_t samplerTableActualBinds = 0;
	std::uint64_t srvAllocationLookups = 0;
	std::uint64_t srvFreeListHits = 0;
	std::uint64_t srvAllocations = 0;
	std::uint64_t srvFreshAllocations = 0;
	std::uint64_t srvInvalidationsRetired = 0;
	std::uint64_t srvDescriptorsReclaimed = 0;
	std::uint64_t retainedResourceRequests = 0;
	std::uint64_t retainedResourceDuplicates = 0;
	std::uint64_t retainedResourceReferences = 0;
	std::uint64_t reclaimedResourceReferences = 0;
	std::uint64_t retainedResidentAllocationReferences = 0;
	std::uint64_t reclaimedResidentAllocationReferences = 0;
	std::uint64_t transitionsEmitted = 0;
	std::uint64_t transitionsSkipped = 0;
	std::uint64_t renderTargetPassEmulationBegins = 0;
	std::uint64_t renderTargetPassEmulationEnds = 0;
	std::uint64_t offscreenPassEmulations = 0;
	std::uint64_t partialClearPassEmulations = 0;
	std::uint64_t presentPassEmulations = 0;
	std::uint64_t fenceWaitChecks = 0;
	std::uint64_t fenceWaits = 0;
	std::uint64_t fenceWaitNanoseconds = 0;
	std::uint64_t flushDrains = 0;
	std::uint64_t finishDrains = 0;
	bool enabled = false;
};
#endif

class LogicalNameAllocator
{
public:
	unsigned int allocate()
	{
		if (names.size() == static_cast<std::size_t>(std::numeric_limits<unsigned int>::max()))
			return 0;
		while (names.find(nextName) != names.end())
			advance();
		const unsigned int name = nextName;
		names.insert(name);
		advance();
		return name;
	}

	void reserve(unsigned int name)
	{
		if (name != 0)
			names.insert(name);
	}

	void release(unsigned int name)
	{
		if (name != 0)
			names.erase(name);
	}

private:
	void advance()
	{
		nextName = nextName == std::numeric_limits<unsigned int>::max() ? 1 : nextName + 1;
	}

	std::set<unsigned int> names;
	unsigned int nextName = 1;
};

struct State
{
	ComPtr<IDXGIFactory6> factory;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<IDXGISwapChain3> swapChain;
	std::array<ComPtr<ID3D12Resource>, D3D12_FRAMES_IN_FLIGHT> backBuffers;
	std::array<FrameResources, D3D12_FRAMES_IN_FLIGHT> frames;
	UINT frameSlot = 0;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	CommandListState commandListState;
	bool commandListOpen = false;

	ComPtr<ID3D12Fence> fence;
	UINT64 nextFenceValue = 1;
	HANDLE fenceEvent = nullptr;

	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> dsvHeap;
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	UINT rtvDescriptorSize = 0;
	UINT srvDescriptorSize = 0;
	UINT samplerDescriptorSize = 0;
	UINT nextSrvDescriptor = D3D12_FIRST_TEXTURE_SRV_DESCRIPTOR;
	std::vector<UINT> freeSrvDescriptors;
	UINT nextSamplerDescriptor = 0;

	ComPtr<ID3D12Resource> colorTarget;
	ComPtr<ID3D12Resource> depthTarget;
	D3D12_RESOURCE_STATES colorTargetState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	int targetWidth = 0;
	int targetHeight = 0;
	bool drawableSizeCheckedForFrame = false;
	bool allowTearing = false;
	UINT swapChainFlags = 0;

	ComPtr<ID3D12RootSignature> legacyRootSignature;
	ComPtr<ID3D12RootSignature> simpleRootSignature;
	ComPtr<ID3DBlob> legacyVertexShader;
	ComPtr<ID3DBlob> legacyPixelShader;
	ComPtr<ID3DBlob> legacyLogicPixelShader;
	ComPtr<ID3DBlob> clearVertexShader;
	ComPtr<ID3DBlob> clearPixelShader;
	ComPtr<ID3DBlob> presentVertexShader;
	ComPtr<ID3DBlob> presentPixelShader;
	ComPtr<ID3D12PipelineState> presentPipeline;
	std::array<ComPtr<ID3D12PipelineState>, 16> clearPipelines;
	std::map<PipelineKey, ComPtr<ID3D12PipelineState>> pipelines;

	TextureStorage fallbackTexture;
	std::map<unsigned int, D3D12Texture> textures;
	std::map<SamplerKey, UINT> samplers;
	SamplerKey currentSamplerKey;
	UINT currentSamplerIndex = 0;
	bool currentSamplerValid = false;
	std::vector<std::unique_ptr<ResidentPage>> residentPages;
	std::unordered_map<std::uint64_t, ResidentGeometryEntry> residentGeometry;
	std::uint64_t residentGeometryCacheHits = 0;
	std::uint64_t residentGeometryCacheMisses = 0;
	UINT64 residentGeometryBytes = 0;
	UINT64 residentGeometryPeakBytes = 0;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	Diagnostics diagnostics;
	std::string pipelineKeyDumpPath;
#endif

	ComPtr<ID3D12InfoQueue> infoQueue;
	UINT64 debugMessagesRead = 0;
	std::uint64_t validationErrors = 0;
	std::uint64_t validationWarnings = 0;
	bool validation = false;
	bool initialized = false;
	bool lineWidthFallbackReported = false;
};

static State state;

static bool sameViewport(const D3D12_VIEWPORT &left, const D3D12_VIEWPORT &right)
{
	return left.TopLeftX == right.TopLeftX && left.TopLeftY == right.TopLeftY &&
		left.Width == right.Width && left.Height == right.Height &&
		left.MinDepth == right.MinDepth && left.MaxDepth == right.MaxDepth;
}

static bool sameScissor(const D3D12_RECT &left, const D3D12_RECT &right)
{
	return left.left == right.left && left.top == right.top &&
		left.right == right.right && left.bottom == right.bottom;
}

static void invalidateCommandListState()
{
	state.commandListState = CommandListState();
}

static void bindLegacyRootSignature()
{
	ID3D12RootSignature *rootSignature = state.legacyRootSignature.Get();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.rootSignatureRequests++;
#endif
	if (state.commandListState.rootSignatureValid &&
		state.commandListState.rootSignature == rootSignature)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.rootSignatureCurrentHits++;
#endif
		return;
	}
	state.commandListState.samplerTableValid = false;
	state.commandList->SetGraphicsRootSignature(rootSignature);
	state.commandListState.rootSignature = rootSignature;
	state.commandListState.rootSignatureValid = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.rootSignatureActualBinds++;
#endif
}

static void bindLegacySamplerTable(D3D12_GPU_DESCRIPTOR_HANDLE sampler)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.samplerTableRequests++;
#endif
	if (state.commandListState.samplerTableValid &&
		state.commandListState.samplerTable.ptr == sampler.ptr)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.samplerTableCurrentHits++;
#endif
		return;
	}
	state.commandList->SetGraphicsRootDescriptorTable(2, sampler);
	state.commandListState.samplerTable = sampler;
	state.commandListState.samplerTableValid = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.samplerTableActualBinds++;
#endif
}

static void bindLegacyTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.topologyRequests++;
#endif
	if (state.commandListState.topologyValid && state.commandListState.topology == topology)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.topologyCurrentHits++;
#endif
		return;
	}
	state.commandList->IASetPrimitiveTopology(topology);
	state.commandListState.topology = topology;
	state.commandListState.topologyValid = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.topologyActualBinds++;
#endif
}

static void bindLegacyDynamicState(const D3D12_VIEWPORT &viewport, const D3D12_RECT &scissor)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
	{
		state.diagnostics.viewportRequests++;
		state.diagnostics.scissorRequests++;
	}
#endif
	if (state.commandListState.viewportValid &&
		sameViewport(state.commandListState.viewport, viewport))
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.viewportCurrentHits++;
#endif
	}
	else
	{
		state.commandList->RSSetViewports(1, &viewport);
		state.commandListState.viewport = viewport;
		state.commandListState.viewportValid = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.viewportActualSets++;
#endif
	}
	if (state.commandListState.scissorValid && sameScissor(state.commandListState.scissor, scissor))
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.scissorCurrentHits++;
#endif
	}
	else
	{
		state.commandList->RSSetScissorRects(1, &scissor);
		state.commandListState.scissor = scissor;
		state.commandListState.scissorValid = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.scissorActualSets++;
#endif
	}
}

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
static void observeRenderTargetPassEmulationBegin(RenderTargetPassEmulation pass)
{
	if (!state.diagnostics.enabled)
		return;
	state.diagnostics.renderTargetPassEmulationBegins++;
	switch (pass)
	{
	case RenderTargetPassEmulation::Offscreen:
		state.diagnostics.offscreenPassEmulations++;
		break;
	case RenderTargetPassEmulation::PartialClear:
		state.diagnostics.partialClearPassEmulations++;
		break;
	case RenderTargetPassEmulation::Present:
		state.diagnostics.presentPassEmulations++;
		break;
	}
}

static void observeRenderTargetPassEmulationEnd()
{
	if (state.diagnostics.enabled)
		state.diagnostics.renderTargetPassEmulationEnds++;
}
#endif

static std::uint32_t floatBits(float value)
{
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static UINT64 alignSize(UINT64 value, UINT64 alignment)
{
	return (value + alignment - 1) / alignment * alignment;
}

static void requireSuccess(HRESULT result, const char *operation)
{
	if (SUCCEEDED(result))
		return;
	char message[192];
	std::snprintf(message, sizeof(message), "%s failed with HRESULT 0x%08lx",
		operation, static_cast<unsigned long>(result));
	throw std::runtime_error(message);
}

static D3D12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE type)
{
	D3D12_HEAP_PROPERTIES properties = {};
	properties.Type = type;
	properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	properties.CreationNodeMask = 1;
	properties.VisibleNodeMask = 1;
	return properties;
}

static D3D12_RESOURCE_DESC bufferDescription(UINT64 size)
{
	D3D12_RESOURCE_DESC description = {};
	description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	description.Width = size;
	description.Height = 1;
	description.DepthOrArraySize = 1;
	description.MipLevels = 1;
	description.SampleDesc.Count = 1;
	description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	return description;
}

static D3D12_RESOURCE_DESC textureDescription(int width, int height, DXGI_FORMAT format,
	D3D12_RESOURCE_FLAGS flags)
{
	D3D12_RESOURCE_DESC description = {};
	description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	description.Width = static_cast<UINT64>(width);
	description.Height = static_cast<UINT>(height);
	description.DepthOrArraySize = 1;
	description.MipLevels = 1;
	description.Format = format;
	description.SampleDesc.Count = 1;
	description.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	description.Flags = flags;
	return description;
}

static D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor(ID3D12DescriptorHeap *heap, UINT index, UINT size)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handle = heap->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<SIZE_T>(index) * static_cast<SIZE_T>(size);
	return handle;
}

static D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor(ID3D12DescriptorHeap *heap, UINT index, UINT size)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handle = heap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += static_cast<UINT64>(index) * static_cast<UINT64>(size);
	return handle;
}

static FrameResources &currentFrame()
{
	return state.frames[state.frameSlot];
}

static void retainResource(const ComPtr<ID3D12Resource> &resource)
{
	if (resource == nullptr)
		return;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.retainedResourceRequests++;
#endif
	FrameResources &frame = currentFrame();
	if (!frame.retainedResourceSet.insert(resource.Get()).second)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.retainedResourceDuplicates++;
#endif
		return;
	}
	frame.retainedResources.push_back(resource);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.retainedResourceReferences++;
#endif
}

static void retainResidentAllocation(const std::shared_ptr<ResidentAllocation> &allocation)
{
	if (allocation == nullptr)
		return;
	FrameResources &frame = currentFrame();
	if (!frame.retainedAllocationSet.insert(allocation.get()).second)
		return;
	frame.residentAllocations.push_back(allocation);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.retainedResidentAllocationReferences++;
#endif
}

static bool collectDebugMessages()
{
	if (!state.validation || state.infoQueue == nullptr)
		return false;

	bool errors = false;
	const UINT64 count = state.infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();
	for (UINT64 index = state.debugMessagesRead; index < count; index++)
	{
		SIZE_T size = 0;
		if (FAILED(state.infoQueue->GetMessage(index, nullptr, &size)) || size == 0)
			continue;
		std::vector<unsigned char> bytes(size);
		D3D12_MESSAGE *message = reinterpret_cast<D3D12_MESSAGE *>(bytes.data());
		if (FAILED(state.infoQueue->GetMessage(index, message, &size)))
			continue;
		if (message->Severity == D3D12_MESSAGE_SEVERITY_CORRUPTION ||
			message->Severity == D3D12_MESSAGE_SEVERITY_ERROR)
		{
			state.validationErrors++;
			errors = true;
			std::fprintf(stderr, "D3D12 validation error %u: %s\n",
				static_cast<unsigned int>(message->ID), message->pDescription);
		}
		else if (message->Severity == D3D12_MESSAGE_SEVERITY_WARNING)
		{
			state.validationWarnings++;
			std::fprintf(stderr, "D3D12 validation warning %u: %s\n",
				static_cast<unsigned int>(message->ID), message->pDescription);
		}
	}
	state.debugMessagesRead = count;
	return errors;
}

static void requireNoDebugErrors()
{
	if (collectDebugMessages())
		throw std::runtime_error("D3D12 validation reported an error");
}

static ComPtr<ID3DBlob> compileShader(const char *source, const char *name, const char *profile,
	const char *entry = "main")
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_IEEE_STRICTNESS;
#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
	ComPtr<ID3DBlob> shader;
	ComPtr<ID3DBlob> diagnostics;
	const HRESULT result = D3DCompile(source, std::strlen(source), name, nullptr, nullptr,
		entry, profile, flags, 0, &shader, &diagnostics);
	if (diagnostics != nullptr && diagnostics->GetBufferSize() != 0)
	{
		const char *text = static_cast<const char *>(diagnostics->GetBufferPointer());
		if (FAILED(result))
			std::fprintf(stderr, "D3D12 shader compile failed (%s): %s\n", name, text);
		else
			std::fprintf(stderr, "D3D12 shader compiler (%s): %s\n", name, text);
	}
	requireSuccess(result, name);
	return shader;
}

static void createDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC description = {};
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	description.NumDescriptors = D3D12_FRAMES_IN_FLIGHT + 2;
	requireSuccess(state.device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&state.rtvHeap)),
		"CreateDescriptorHeap(RTV)");

	description = {};
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	description.NumDescriptors = 1;
	requireSuccess(state.device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&state.dsvHeap)),
		"CreateDescriptorHeap(DSV)");

	description = {};
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	description.NumDescriptors = D3D12_SRV_DESCRIPTOR_COUNT;
	description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	requireSuccess(state.device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&state.srvHeap)),
		"CreateDescriptorHeap(SRV)");

	description = {};
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	description.NumDescriptors = D3D12_SAMPLER_DESCRIPTOR_COUNT;
	description.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	requireSuccess(state.device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&state.samplerHeap)),
		"CreateDescriptorHeap(sampler)");

	state.rtvDescriptorSize = state.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	state.srvDescriptorSize = state.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	state.samplerDescriptorSize = state.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
}

static UploadChunk createUploadChunk(UINT64 size)
{
	UploadChunk chunk;
	chunk.size = size;
	const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_UPLOAD);
	const D3D12_RESOURCE_DESC description = bufferDescription(size);
	requireSuccess(state.device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
		&description, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&chunk.resource)), "CreateCommittedResource(upload)");
	D3D12_RANGE readRange = { 0, 0 };
	requireSuccess(chunk.resource->Map(0, &readRange, reinterpret_cast<void **>(&chunk.mapped)),
		"Map(upload)");
	return chunk;
}

static UploadAllocation allocateUpload(UINT64 size, UINT64 alignment)
{
	FrameResources &frame = currentFrame();
	for (UploadChunk &chunk : frame.uploadChunks)
	{
		const UINT64 offset = alignSize(chunk.used, alignment);
		if (offset <= chunk.size && size <= chunk.size - offset)
		{
			chunk.used = offset + size;
			UploadAllocation allocation;
			allocation.resource = chunk.resource.Get();
			allocation.offset = offset;
			allocation.gpuAddress = chunk.resource->GetGPUVirtualAddress() + offset;
			allocation.mapped = chunk.mapped + offset;
			return allocation;
		}
	}

	frame.uploadChunks.push_back(createUploadChunk(std::max(D3D12_UPLOAD_CHUNK_SIZE,
		alignSize(size, alignment))));
	UploadChunk &chunk = frame.uploadChunks.back();
	chunk.used = size;
	UploadAllocation allocation;
	allocation.resource = chunk.resource.Get();
	allocation.gpuAddress = chunk.resource->GetGPUVirtualAddress();
	allocation.mapped = chunk.mapped;
	return allocation;
}

static D3D12_RASTERIZER_DESC defaultRasterizer()
{
	D3D12_RASTERIZER_DESC description = {};
	description.FillMode = D3D12_FILL_MODE_SOLID;
	description.CullMode = D3D12_CULL_MODE_NONE;
	description.FrontCounterClockwise = FALSE;
	description.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	description.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	description.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	description.DepthClipEnable = TRUE;
	description.MultisampleEnable = FALSE;
	description.AntialiasedLineEnable = FALSE;
	description.ForcedSampleCount = 0;
	description.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return description;
}

static D3D12_BLEND_DESC defaultBlend()
{
	D3D12_BLEND_DESC description = {};
	description.AlphaToCoverageEnable = FALSE;
	description.IndependentBlendEnable = FALSE;
	D3D12_RENDER_TARGET_BLEND_DESC &target = description.RenderTarget[0];
	target.BlendEnable = FALSE;
	target.LogicOpEnable = FALSE;
	target.SrcBlend = D3D12_BLEND_ONE;
	target.DestBlend = D3D12_BLEND_ZERO;
	target.BlendOp = D3D12_BLEND_OP_ADD;
	target.SrcBlendAlpha = D3D12_BLEND_ONE;
	target.DestBlendAlpha = D3D12_BLEND_ZERO;
	target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	target.LogicOp = D3D12_LOGIC_OP_NOOP;
	target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	return description;
}

static D3D12_DEPTH_STENCIL_DESC disabledDepthStencil()
{
	D3D12_DEPTH_STENCIL_DESC description = {};
	description.DepthEnable = FALSE;
	description.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	description.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	description.StencilEnable = FALSE;
	description.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	description.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	description.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	description.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	description.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	description.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	description.BackFace = description.FrontFace;
	return description;
}

static ComPtr<ID3D12RootSignature> createRootSignature(const D3D12_ROOT_SIGNATURE_DESC &description,
	const char *operation)
{
	ComPtr<ID3DBlob> serialized;
	ComPtr<ID3DBlob> diagnostics;
	const HRESULT result = D3D12SerializeRootSignature(&description, D3D_ROOT_SIGNATURE_VERSION_1,
		&serialized, &diagnostics);
	if (diagnostics != nullptr && diagnostics->GetBufferSize() != 0)
	{
		std::fprintf(stderr, "D3D12 root signature (%s): %s\n", operation,
			static_cast<const char *>(diagnostics->GetBufferPointer()));
	}
	requireSuccess(result, operation);
	ComPtr<ID3D12RootSignature> rootSignature;
	requireSuccess(state.device->CreateRootSignature(0, serialized->GetBufferPointer(),
		serialized->GetBufferSize(), IID_PPV_ARGS(&rootSignature)), operation);
	return rootSignature;
}

static void createRootSignatures()
{
	D3D12_DESCRIPTOR_RANGE legacyRanges[2] = {};
	legacyRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	legacyRanges[0].NumDescriptors = 1;
	legacyRanges[0].BaseShaderRegister = 0;
	legacyRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	legacyRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	legacyRanges[1].NumDescriptors = 1;
	legacyRanges[1].BaseShaderRegister = 0;
	legacyRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER legacyParameters[3] = {};
	legacyParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	legacyParameters[0].Descriptor.ShaderRegister = 0;
	legacyParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	legacyParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	legacyParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	legacyParameters[1].DescriptorTable.pDescriptorRanges = &legacyRanges[0];
	legacyParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	legacyParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	legacyParameters[2].DescriptorTable.NumDescriptorRanges = 1;
	legacyParameters[2].DescriptorTable.pDescriptorRanges = &legacyRanges[1];
	legacyParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC legacyDescription = {};
	legacyDescription.NumParameters = 3;
	legacyDescription.pParameters = legacyParameters;
	legacyDescription.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	state.legacyRootSignature = createRootSignature(legacyDescription, "CreateRootSignature(legacy)");

	D3D12_DESCRIPTOR_RANGE simpleRange = {};
	simpleRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	simpleRange.NumDescriptors = 1;
	simpleRange.BaseShaderRegister = 0;
	simpleRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	D3D12_ROOT_PARAMETER simpleParameters[2] = {};
	simpleParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	simpleParameters[0].Constants.ShaderRegister = 0;
	simpleParameters[0].Constants.Num32BitValues = 4;
	simpleParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	simpleParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	simpleParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	simpleParameters[1].DescriptorTable.pDescriptorRanges = &simpleRange;
	simpleParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	D3D12_ROOT_SIGNATURE_DESC simpleDescription = {};
	simpleDescription.NumParameters = 2;
	simpleDescription.pParameters = simpleParameters;
	simpleDescription.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	state.simpleRootSignature = createRootSignature(simpleDescription, "CreateRootSignature(simple)");
}

static D3D12_SHADER_BYTECODE shaderBytecode(ID3DBlob *shader)
{
	D3D12_SHADER_BYTECODE bytecode = {};
	bytecode.pShaderBytecode = shader->GetBufferPointer();
	bytecode.BytecodeLength = shader->GetBufferSize();
	return bytecode;
}

static ComPtr<ID3D12PipelineState> createSimplePipeline(ID3DBlob *vertexShader,
	ID3DBlob *pixelShader, unsigned int colorWriteMask)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
	description.pRootSignature = state.simpleRootSignature.Get();
	description.VS = shaderBytecode(vertexShader);
	description.PS = shaderBytecode(pixelShader);
	description.BlendState = defaultBlend();
	description.BlendState.RenderTarget[0].RenderTargetWriteMask =
		static_cast<UINT8>(colorWriteMask);
	description.SampleMask = UINT_MAX;
	description.RasterizerState = defaultRasterizer();
	description.DepthStencilState = disabledDepthStencil();
	description.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	description.NumRenderTargets = 1;
	description.RTVFormats[0] = D3D12_COLOR_FORMAT;
	description.SampleDesc.Count = 1;
	ComPtr<ID3D12PipelineState> pipeline;
	requireSuccess(state.device->CreateGraphicsPipelineState(&description, IID_PPV_ARGS(&pipeline)),
		"CreateGraphicsPipelineState(simple)");
	return pipeline;
}

static void createShadersAndPipelines()
{
	state.legacyVertexShader = compileShader(legacyVertexShaderSource(), "LegacyFFP VS", "vs_5_1");
	state.legacyPixelShader = compileShader(legacyPixelShaderSource(), "LegacyFFP PS", "ps_5_1");
	state.legacyLogicPixelShader = compileShader(legacyPixelShaderSource(), "LegacyFFP logic PS",
		"ps_5_1", "mainLogic");
	state.clearVertexShader = compileShader(clearVertexShaderSource(), "Clear VS", "vs_5_1");
	state.clearPixelShader = compileShader(clearPixelShaderSource(), "Clear PS", "ps_5_1");
	state.presentVertexShader = compileShader(presentVertexShaderSource(), "Present VS", "vs_5_1");
	state.presentPixelShader = compileShader(presentPixelShaderSource(), "Present PS", "ps_5_1");
	state.presentPipeline = createSimplePipeline(state.presentVertexShader.Get(),
		state.presentPixelShader.Get(), D3D12_COLOR_WRITE_ENABLE_ALL);
}

static bool supportsFormat(ID3D12Device *device, DXGI_FORMAT format,
	D3D12_FORMAT_SUPPORT1 required)
{
	D3D12_FEATURE_DATA_FORMAT_SUPPORT support = {};
	support.Format = format;
	return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,
		&support, sizeof(support))) && (support.Support1 & required) == required;
}

static bool supportsRequiredDeviceFeatures(ID3D12Device *device)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
	const D3D12_FORMAT_SUPPORT1 colorResourceRequired =
		static_cast<D3D12_FORMAT_SUPPORT1>(D3D12_FORMAT_SUPPORT1_TEXTURE2D);
	const D3D12_FORMAT_SUPPORT1 colorRequired = static_cast<D3D12_FORMAT_SUPPORT1>(
		D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE |
		D3D12_FORMAT_SUPPORT1_RENDER_TARGET | D3D12_FORMAT_SUPPORT1_BLENDABLE);
	const D3D12_FORMAT_SUPPORT1 logicColorRequired = static_cast<D3D12_FORMAT_SUPPORT1>(
		D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
	const D3D12_FORMAT_SUPPORT1 depthRequired = static_cast<D3D12_FORMAT_SUPPORT1>(
		D3D12_FORMAT_SUPPORT1_TEXTURE2D | D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
	return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS,
		&options, sizeof(options))) && options.OutputMergerLogicOp &&
		supportsFormat(device, D3D12_COLOR_RESOURCE_FORMAT, colorResourceRequired) &&
		supportsFormat(device, D3D12_COLOR_FORMAT, colorRequired) &&
		supportsFormat(device, D3D12_LOGIC_COLOR_FORMAT, logicColorRequired) &&
		supportsFormat(device, D3D12_DEPTH_FORMAT, depthRequired);
}

static void createDevice()
{
	const char *validate = std::getenv("A126_LEGACYGL_VALIDATE");
	state.validation = validate != nullptr && std::strcmp(validate, "1") == 0;
	if (state.validation)
	{
		ComPtr<ID3D12Debug> debug;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
			debug->EnableDebugLayer();
		else
			std::fprintf(stderr, "D3D12 validation: debug layer is unavailable\n");
	}

	UINT factoryFlags = state.validation ? DXGI_CREATE_FACTORY_DEBUG : 0;
	if (FAILED(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&state.factory))))
	{
		factoryFlags = 0;
		requireSuccess(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&state.factory)),
			"CreateDXGIFactory2");
	}

	for (UINT index = 0; ; index++)
	{
		ComPtr<IDXGIAdapter1> adapter;
		const HRESULT enumerateResult = state.factory->EnumAdapterByGpuPreference(index,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter));
		if (FAILED(enumerateResult))
			break;
		DXGI_ADAPTER_DESC1 description = {};
		adapter->GetDesc1(&description);
		if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			continue;
		ComPtr<ID3D12Device> device;
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device))) && supportsRequiredDeviceFeatures(device.Get()))
		{
			state.device = device;
			std::wcout << L"d3d12: adapter=" << description.Description << L'\n';
			break;
		}
	}
	if (state.device == nullptr)
	{
		ComPtr<IDXGIAdapter> warp;
		requireSuccess(state.factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)), "EnumWarpAdapter");
		ComPtr<ID3D12Device> device;
		requireSuccess(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device)), "D3D12CreateDevice(WARP)");
		if (!supportsRequiredDeviceFeatures(device.Get()))
			throw std::runtime_error("D3D12 WARP device lacks required logic-op or format support");
		state.device = device;
		std::cout << "d3d12: adapter=WARP\n";
	}
	std::cout << "d3d12: capability_report feature_level=11_0"
		" color_resource=DXGI_FORMAT_R8G8B8A8_TYPELESS"
		" texture_storage=DXGI_FORMAT_R8G8B8A8_UNORM sampled=native"
		" render_target=native logic_op=native line_width=width1-fallback"
		" legacy_dither=unavailable-no-emulation resource_state=tracked\n";

	if (state.validation && SUCCEEDED(state.device.As(&state.infoQueue)))
	{
		D3D12_MESSAGE_SEVERITY denied[] = {
			D3D12_MESSAGE_SEVERITY_INFO,
			D3D12_MESSAGE_SEVERITY_MESSAGE
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumSeverities = 2;
		filter.DenyList.pSeverityList = denied;
		state.infoQueue->PushStorageFilter(&filter);
	}

	D3D12_COMMAND_QUEUE_DESC queueDescription = {};
	queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	requireSuccess(state.device->CreateCommandQueue(&queueDescription,
		IID_PPV_ARGS(&state.commandQueue)), "CreateCommandQueue");
	for (FrameResources &frame : state.frames)
	{
		requireSuccess(state.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&frame.allocator)), "CreateCommandAllocator");
	}
	requireSuccess(state.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		state.frames[0].allocator.Get(), nullptr, IID_PPV_ARGS(&state.commandList)),
		"CreateCommandList");
	requireSuccess(state.commandList->Close(), "Close(initial command list)");
	requireSuccess(state.device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&state.fence)), "CreateFence");
	state.fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (state.fenceEvent == nullptr)
		throw std::runtime_error("CreateEventW for D3D12 fence failed");
}

static void waitForFence(UINT64 value)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.fenceWaitChecks++;
#endif
	if (value == 0 || state.fence->GetCompletedValue() >= value)
		return;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	const std::chrono::steady_clock::time_point start = state.diagnostics.enabled ?
		std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point();
#endif
	requireSuccess(state.fence->SetEventOnCompletion(value, state.fenceEvent),
		"SetEventOnCompletion");
	if (WaitForSingleObject(state.fenceEvent, INFINITE) != WAIT_OBJECT_0)
		throw std::runtime_error("waiting for the D3D12 fence failed");
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
	{
		state.diagnostics.fenceWaits++;
		state.diagnostics.fenceWaitNanoseconds += static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::steady_clock::now() - start).count());
	}
#endif
}

static void beginFrame()
{
	FrameResources &frame = currentFrame();
	waitForFence(frame.fenceValue);
	frame.fenceValue = 0;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
	{
		state.diagnostics.srvDescriptorsReclaimed += frame.retiredSrvDescriptors.size();
		state.diagnostics.reclaimedResourceReferences += frame.retainedResources.size();
		state.diagnostics.reclaimedResidentAllocationReferences += frame.residentAllocations.size();
	}
#endif
	state.freeSrvDescriptors.insert(state.freeSrvDescriptors.end(),
		frame.retiredSrvDescriptors.begin(), frame.retiredSrvDescriptors.end());
	frame.retiredSrvDescriptors.clear();
	frame.retainedResources.clear();
	frame.retainedResourceSet.clear();
	frame.residentAllocations.clear();
	frame.retainedAllocationSet.clear();
	for (UploadChunk &chunk : frame.uploadChunks)
		chunk.used = 0;
	requireSuccess(frame.allocator->Reset(), "Reset(command allocator)");
	requireSuccess(state.commandList->Reset(frame.allocator.Get(), nullptr), "Reset(command list)");
	invalidateCommandListState();
	state.commandListOpen = true;
	state.drawableSizeCheckedForFrame = false;
}

static void closeAndExecute()
{
	if (!state.commandListOpen)
		return;
	requireSuccess(state.commandList->Close(), "Close(command list)");
	ID3D12CommandList *lists[] = { state.commandList.Get() };
	state.commandQueue->ExecuteCommandLists(1, lists);
	state.commandListOpen = false;
}

static UINT64 signalCurrentFrame()
{
	const UINT64 value = state.nextFenceValue++;
	requireSuccess(state.commandQueue->Signal(state.fence.Get(), value), "Signal(frame fence)");
	currentFrame().fenceValue = value;
	return value;
}

static void flushAndWait()
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.flushDrains++;
#endif
	closeAndExecute();
	const UINT64 value = signalCurrentFrame();
	waitForFence(value);
	requireNoDebugErrors();
	beginFrame();
}

static void transitionResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES before,
	D3D12_RESOURCE_STATES after)
{
	if (before == after)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.transitionsSkipped++;
#endif
		return;
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.transitionsEmitted++;
#endif
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = resource;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	state.commandList->ResourceBarrier(1, &barrier);
}

static void transitionTrackedResource(ID3D12Resource *resource,
	D3D12_RESOURCE_STATES &currentState, D3D12_RESOURCE_STATES requiredState)
{
	transitionResource(resource, currentState, requiredState);
	currentState = requiredState;
}

static void createTargetResources(int width, int height)
{
	for (UINT index = 0; index < D3D12_FRAMES_IN_FLIGHT; index++)
	{
		requireSuccess(state.swapChain->GetBuffer(index, IID_PPV_ARGS(&state.backBuffers[index])),
			"GetBuffer(swap chain)");
		state.device->CreateRenderTargetView(state.backBuffers[index].Get(), nullptr,
			cpuDescriptor(state.rtvHeap.Get(), index + 1, state.rtvDescriptorSize));
	}

	const D3D12_HEAP_PROPERTIES defaultHeap = heapProperties(D3D12_HEAP_TYPE_DEFAULT);
	const D3D12_RESOURCE_DESC colorDescription = textureDescription(width, height,
		D3D12_COLOR_RESOURCE_FORMAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	requireSuccess(state.device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
		&colorDescription, D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr,
		IID_PPV_ARGS(&state.colorTarget)), "CreateCommittedResource(color target)");
	state.colorTargetState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	D3D12_RENDER_TARGET_VIEW_DESC colorRenderTargetView = {};
	colorRenderTargetView.Format = D3D12_COLOR_FORMAT;
	colorRenderTargetView.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	state.device->CreateRenderTargetView(state.colorTarget.Get(), &colorRenderTargetView,
		cpuDescriptor(state.rtvHeap.Get(), 0, state.rtvDescriptorSize));
	colorRenderTargetView.Format = D3D12_LOGIC_COLOR_FORMAT;
	state.device->CreateRenderTargetView(state.colorTarget.Get(), &colorRenderTargetView,
		cpuDescriptor(state.rtvHeap.Get(), D3D12_FRAMES_IN_FLIGHT + 1,
			state.rtvDescriptorSize));

	D3D12_SHADER_RESOURCE_VIEW_DESC colorShaderView = {};
	colorShaderView.Format = D3D12_COLOR_FORMAT;
	colorShaderView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	colorShaderView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	colorShaderView.Texture2D.MipLevels = 1;
	state.device->CreateShaderResourceView(state.colorTarget.Get(), &colorShaderView,
		cpuDescriptor(state.srvHeap.Get(), 1, state.srvDescriptorSize));

	const D3D12_RESOURCE_DESC depthDescription = textureDescription(width, height,
		D3D12_DEPTH_FORMAT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	D3D12_CLEAR_VALUE depthClear = {};
	depthClear.Format = D3D12_DEPTH_FORMAT;
	depthClear.DepthStencil.Depth = 1.0f;
	depthClear.DepthStencil.Stencil = 0;
	requireSuccess(state.device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE,
		&depthDescription, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear,
		IID_PPV_ARGS(&state.depthTarget)), "CreateCommittedResource(depth target)");
	D3D12_DEPTH_STENCIL_VIEW_DESC depthView = {};
	depthView.Format = D3D12_DEPTH_FORMAT;
	depthView.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	state.device->CreateDepthStencilView(state.depthTarget.Get(), &depthView,
		state.dsvHeap->GetCPUDescriptorHandleForHeapStart());

	state.targetWidth = width;
	state.targetHeight = height;
}

static void createSwapChain()
{
	BOOL tearing = FALSE;
	if (SUCCEEDED(state.factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&tearing, sizeof(tearing))))
	{
		state.allowTearing = tearing == TRUE;
	}
	state.swapChainFlags = state.allowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	int width = 0;
	int height = 0;
	platform::getDrawableSize(width, height);
	width = std::max(width, 1);
	height = std::max(height, 1);
	DXGI_SWAP_CHAIN_DESC1 description = {};
	description.Width = static_cast<UINT>(width);
	description.Height = static_cast<UINT>(height);
	description.Format = D3D12_COLOR_FORMAT;
	description.SampleDesc.Count = 1;
	description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	description.BufferCount = D3D12_FRAMES_IN_FLIGHT;
	description.Scaling = DXGI_SCALING_STRETCH;
	description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	description.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	description.Flags = state.swapChainFlags;

	HWND window = static_cast<HWND>(platform::getWin32WindowHandle());
	if (window == nullptr)
		throw std::runtime_error("the D3D12 backend did not receive a Win32 window handle");
	ComPtr<IDXGISwapChain1> swapChain;
	requireSuccess(state.factory->CreateSwapChainForHwnd(state.commandQueue.Get(), window,
		&description, nullptr, nullptr, &swapChain), "CreateSwapChainForHwnd");
	requireSuccess(swapChain.As(&state.swapChain), "QueryInterface(IDXGISwapChain3)");
	requireSuccess(state.factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER),
		"MakeWindowAssociation");
	createTargetResources(width, height);
	std::cout << "d3d12: present=" << (state.allowTearing ? "immediate-tearing" : "immediate") <<
		", frames=" << D3D12_FRAMES_IN_FLIGHT << '\n';
}

static bool ensureRenderTargets()
{
	if (state.drawableSizeCheckedForFrame)
		return state.targetWidth > 0 && state.targetHeight > 0;
	state.drawableSizeCheckedForFrame = true;
	int width = 0;
	int height = 0;
	platform::getDrawableSize(width, height);
	if (width <= 0 || height <= 0)
		return false;
	if (width == state.targetWidth && height == state.targetHeight)
		return true;

	flushAndWait();
	for (ComPtr<ID3D12Resource> &backBuffer : state.backBuffers)
		backBuffer.Reset();
	state.colorTarget.Reset();
	state.depthTarget.Reset();
	requireSuccess(state.swapChain->ResizeBuffers(D3D12_FRAMES_IN_FLIGHT,
		static_cast<UINT>(width), static_cast<UINT>(height), D3D12_COLOR_FORMAT,
		state.swapChainFlags), "ResizeBuffers");
	createTargetResources(width, height);
	state.drawableSizeCheckedForFrame = true;
	return true;
}

static D3D12_VIEWPORT fullViewport()
{
	D3D12_VIEWPORT viewport = {};
	viewport.Width = static_cast<float>(state.targetWidth);
	viewport.Height = static_cast<float>(state.targetHeight);
	viewport.MaxDepth = 1.0f;
	return viewport;
}

static D3D12_RECT fullScissor()
{
	D3D12_RECT rectangle = {};
	rectangle.right = state.targetWidth;
	rectangle.bottom = state.targetHeight;
	return rectangle;
}

static void bindOffscreenTargets(bool withDepth, bool logicOp = false)
{
	const UINT colorDescriptor = logicOp ? D3D12_FRAMES_IN_FLIGHT + 1 : 0;
	const D3D12_CPU_DESCRIPTOR_HANDLE color = cpuDescriptor(state.rtvHeap.Get(), colorDescriptor,
		state.rtvDescriptorSize);
	const D3D12_CPU_DESCRIPTOR_HANDLE depth = state.dsvHeap->GetCPUDescriptorHandleForHeapStart();
	state.commandList->OMSetRenderTargets(1, &color, FALSE, withDepth ? &depth : nullptr);
}

static std::size_t alignedRowSize(std::size_t rowSize, int alignment)
{
	const std::size_t value = static_cast<std::size_t>(alignment);
	return (rowSize + value - 1) / value * value;
}

static unsigned char floatByte(float value)
{
	const float clamped = std::max(0.0f, std::min(1.0f, value));
	return static_cast<unsigned char>(clamped * 255.0f + 0.5f);
}

static UINT allocateSrvDescriptor()
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.srvAllocationLookups++;
#endif
	if (!state.freeSrvDescriptors.empty())
	{
		const UINT index = state.freeSrvDescriptors.back();
		state.freeSrvDescriptors.pop_back();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
		{
			state.diagnostics.srvFreeListHits++;
			state.diagnostics.srvAllocations++;
		}
#endif
		return index;
	}
	if (state.nextSrvDescriptor >= D3D12_SRV_DESCRIPTOR_COUNT)
		throw std::runtime_error("D3D12 shader-resource descriptor heap is exhausted");
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
	{
		state.diagnostics.srvAllocations++;
		state.diagnostics.srvFreshAllocations++;
	}
#endif
	return state.nextSrvDescriptor++;
}

static void retireSrvDescriptor(UINT index)
{
	if (index >= D3D12_FIRST_TEXTURE_SRV_DESCRIPTOR)
	{
		currentFrame().retiredSrvDescriptors.push_back(index);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.srvInvalidationsRetired++;
#endif
	}
}

static void uploadTexture(TextureStorage &storage, const unsigned char *rgba, int width, int height)
{
	retainResource(storage.resource);
	const UINT rowPitch = static_cast<UINT>(alignSize(static_cast<UINT64>(width) * 4,
		D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	const UINT64 uploadSize = static_cast<UINT64>(rowPitch) * static_cast<UINT64>(height);
	const UploadAllocation upload = allocateUpload(uploadSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
	for (int y = 0; y < height; y++)
	{
		std::memcpy(static_cast<unsigned char *>(upload.mapped) +
			static_cast<std::size_t>(y) * rowPitch,
			rgba + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 4,
			static_cast<std::size_t>(width) * 4);
	}

	transitionTrackedResource(storage.resource.Get(), storage.resourceState,
		D3D12_RESOURCE_STATE_COPY_DEST);
	D3D12_TEXTURE_COPY_LOCATION destination = {};
	destination.pResource = storage.resource.Get();
	destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	D3D12_TEXTURE_COPY_LOCATION source = {};
	source.pResource = upload.resource;
	source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	source.PlacedFootprint.Offset = upload.offset;
	source.PlacedFootprint.Footprint.Format = D3D12_COLOR_FORMAT;
	source.PlacedFootprint.Footprint.Width = static_cast<UINT>(width);
	source.PlacedFootprint.Footprint.Height = static_cast<UINT>(height);
	source.PlacedFootprint.Footprint.Depth = 1;
	source.PlacedFootprint.Footprint.RowPitch = rowPitch;
	state.commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
	transitionTrackedResource(storage.resource.Get(), storage.resourceState,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

static void replaceTextureStorage(TextureStorage &storage, const unsigned char *rgba,
	int width, int height, UINT fixedDescriptor = UINT_MAX)
{
	if (storage.resource == nullptr || storage.width != width || storage.height != height)
	{
		if (storage.resource != nullptr)
		{
			retainResource(storage.resource);
			retireSrvDescriptor(storage.srvIndex);
		}
		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_DESC description = textureDescription(width, height,
			D3D12_COLOR_FORMAT, D3D12_RESOURCE_FLAG_NONE);
		ComPtr<ID3D12Resource> resource;
		requireSuccess(state.device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
			&description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&resource)), "CreateCommittedResource(texture)");
		storage.resource = resource;
		storage.resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
		storage.width = width;
		storage.height = height;
		storage.srvIndex = fixedDescriptor == UINT_MAX ? allocateSrvDescriptor() : fixedDescriptor;
		D3D12_SHADER_RESOURCE_VIEW_DESC view = {};
		view.Format = D3D12_COLOR_FORMAT;
		view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		view.Texture2D.MipLevels = 1;
		state.device->CreateShaderResourceView(storage.resource.Get(), &view,
			cpuDescriptor(state.srvHeap.Get(), storage.srvIndex, state.srvDescriptorSize));
	}
	uploadTexture(storage, rgba, width, height);
}

static D3D12_TEXTURE_ADDRESS_MODE textureAddressMode(unsigned int wrap)
{
	return wrap == GL_REPEAT ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
}

static bool baseMinFilterLinear(unsigned int filter)
{
	return filter == GL_LINEAR || filter == GL_LINEAR_MIPMAP_NEAREST ||
		filter == GL_LINEAR_MIPMAP_LINEAR;
}

static D3D12_FILTER textureFilter(unsigned int minFilter, unsigned int magFilter)
{
	const bool minLinear = baseMinFilterLinear(minFilter);
	const bool magLinear = magFilter != GL_NEAREST;
	if (minLinear && magLinear)
		return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	if (minLinear)
		return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	if (magLinear)
		return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	return D3D12_FILTER_MIN_MAG_MIP_POINT;
}

static D3D12_GPU_DESCRIPTOR_HANDLE samplerFor(unsigned int minFilter, unsigned int magFilter,
	unsigned int wrapS, unsigned int wrapT)
{
	SamplerKey key;
	key.minFilter = baseMinFilterLinear(minFilter) ? GL_LINEAR : GL_NEAREST;
	key.magFilter = magFilter == GL_NEAREST ? GL_NEAREST : GL_LINEAR;
	key.wrapS = wrapS;
	key.wrapT = wrapT;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.samplerRequests++;
#endif
	if (state.currentSamplerValid && state.currentSamplerKey == key)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.samplerCurrentHits++;
#endif
		return gpuDescriptor(state.samplerHeap.Get(), state.currentSamplerIndex,
			state.samplerDescriptorSize);
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.samplerMapLookups++;
#endif
	auto found = state.samplers.find(key);
	UINT index = 0;
	if (found != state.samplers.end())
	{
		index = found->second;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.samplerMapHits++;
#endif
	}
	else
	{
		if (state.nextSamplerDescriptor >= D3D12_SAMPLER_DESCRIPTOR_COUNT)
			throw std::runtime_error("D3D12 sampler descriptor heap is exhausted");
		index = state.nextSamplerDescriptor++;
		D3D12_SAMPLER_DESC description = {};
		description.Filter = textureFilter(minFilter, magFilter);
		description.AddressU = textureAddressMode(wrapS);
		description.AddressV = textureAddressMode(wrapT);
		description.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		description.MaxAnisotropy = 1;
		description.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		description.MinLOD = 0.0f;
		description.MaxLOD = 0.0f;
		state.device->CreateSampler(&description,
			cpuDescriptor(state.samplerHeap.Get(), index, state.samplerDescriptorSize));
		state.samplers.insert(std::make_pair(key, index));
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.samplerAllocations++;
#endif
	}
	state.currentSamplerKey = key;
	state.currentSamplerIndex = index;
	state.currentSamplerValid = true;
	return gpuDescriptor(state.samplerHeap.Get(), index, state.samplerDescriptorSize);
}

static void ensureTextureStorage(D3D12Texture &texture,
	const legacygl::ResolvedTextureState &textureState, bool useGutter)
{
	unsigned char border[4];
	for (int i = 0; i < 4; i++)
		border[i] = floatByte(textureState.borderColor[i]);
	if (!texture.derivedDirty && texture.derivedHasGutter == useGutter &&
		(!useGutter || (texture.derivedWrapS == textureState.wrapS &&
		texture.derivedWrapT == textureState.wrapT &&
		std::memcmp(texture.derivedBorder, border, 4) == 0)))
	{
		return;
	}

	TextureLevel &source = texture.levels[0];
	if (!source.defined || source.width <= 0 || source.height <= 0)
		return;
	if (!useGutter)
	{
		replaceTextureStorage(texture.storage, source.rgba.data(), source.width, source.height);
		texture.derivedHasGutter = false;
		texture.derivedDirty = false;
		return;
	}

	const int width = source.width + 2;
	const int height = source.height + 2;
	std::vector<unsigned char> derived(static_cast<std::size_t>(width) *
		static_cast<std::size_t>(height) * 4);
	for (int y = 0; y < height; y++)
	{
		const bool outsideT = y == 0 || y == height - 1;
		int sourceY = y - 1;
		if (y == 0)
			sourceY = textureState.wrapT == GL_REPEAT ? source.height - 1 : 0;
		else if (y == height - 1)
			sourceY = textureState.wrapT == GL_REPEAT ? 0 : source.height - 1;
		for (int x = 0; x < width; x++)
		{
			const bool outsideS = x == 0 || x == width - 1;
			int sourceX = x - 1;
			if (x == 0)
				sourceX = textureState.wrapS == GL_REPEAT ? source.width - 1 : 0;
			else if (x == width - 1)
				sourceX = textureState.wrapS == GL_REPEAT ? 0 : source.width - 1;
			unsigned char *destination = derived.data() +
				(static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
				static_cast<std::size_t>(x)) * 4;
			if ((outsideS && textureState.wrapS == GL_CLAMP) ||
				(outsideT && textureState.wrapT == GL_CLAMP))
			{
				std::memcpy(destination, border, 4);
			}
			else
			{
				std::memcpy(destination, source.rgba.data() +
					(static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(source.width) +
					static_cast<std::size_t>(sourceX)) * 4, 4);
			}
		}
	}
	replaceTextureStorage(texture.storage, derived.data(), width, height);
	texture.derivedWrapS = textureState.wrapS;
	texture.derivedWrapT = textureState.wrapT;
	std::memcpy(texture.derivedBorder, border, 4);
	texture.derivedHasGutter = true;
	texture.derivedDirty = false;
}

static void createFallbackTexture()
{
	const unsigned char white[4] = { 255, 255, 255, 255 };
	replaceTextureStorage(state.fallbackTexture, white, 1, 1, 0);
}

static TextureBinding bindTextureState(const legacygl::ResolvedDraw &command,
	D3D12GPUState &gpuState)
{
	TextureBinding binding;
	if (!command.enables.texture2D || !command.texture.complete)
	{
		binding.srv = gpuDescriptor(state.srvHeap.Get(), state.fallbackTexture.srvIndex,
			state.srvDescriptorSize);
		binding.sampler = samplerFor(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE);
		binding.resource = state.fallbackTexture.resource;
		return binding;
	}

	D3D12Texture &texture = state.textures[command.texture.name];
	TextureLevel &level = texture.levels[0];
	if (!level.defined)
	{
		gpuState.flags3[2] = 0;
		binding.srv = gpuDescriptor(state.srvHeap.Get(), state.fallbackTexture.srvIndex,
			state.srvDescriptorSize);
		binding.sampler = samplerFor(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE,
			GL_CLAMP_TO_EDGE);
		binding.resource = state.fallbackTexture.resource;
		return binding;
	}

	const bool useGutter = command.texture.wrapS == GL_CLAMP || command.texture.wrapT == GL_CLAMP;
	ensureTextureStorage(texture, command.texture, useGutter);
	const unsigned int samplerWrapS = useGutter ? GL_CLAMP_TO_EDGE : command.texture.wrapS;
	const unsigned int samplerWrapT = useGutter ? GL_CLAMP_TO_EDGE : command.texture.wrapT;
	binding.srv = gpuDescriptor(state.srvHeap.Get(), texture.storage.srvIndex,
		state.srvDescriptorSize);
	binding.sampler = samplerFor(command.texture.minFilter, command.texture.magFilter,
		samplerWrapS, samplerWrapT);
	binding.resource = texture.storage.resource;
	gpuState.textureSize[0] = static_cast<float>(level.width);
	gpuState.textureSize[1] = static_cast<float>(level.height);
	gpuState.textureSize[2] = static_cast<float>(level.width + 2);
	gpuState.textureSize[3] = static_cast<float>(level.height + 2);
	gpuState.flags3[3] = useGutter ? 1u : 0u;
	return binding;
}

static void copy4(float *destination, const float *source)
{
	std::memcpy(destination, source, 4 * sizeof(float));
}

static void copyMaterial(D3D12GPUMaterial &destination, const legacygl::MaterialState &source)
{
	copy4(destination.ambient, source.ambient);
	copy4(destination.diffuse, source.diffuse);
	copy4(destination.specular, source.specular);
	copy4(destination.emission, source.emission);
	destination.shininess[0] = source.shininess;
}

static unsigned int alphaFunction(unsigned int function)
{
	switch (function)
	{
		case GL_NEVER: return 0;
		case GL_LESS: return 1;
		case GL_EQUAL: return 2;
		case GL_LEQUAL: return 3;
		case GL_GREATER: return 4;
		case GL_NOTEQUAL: return 5;
		case GL_GEQUAL: return 6;
		default: return 7;
	}
}

static unsigned int fogMode(unsigned int mode, bool enabled)
{
	if (!enabled)
		return 0;
	if (mode == GL_LINEAR)
		return 1;
	if (mode == GL_EXP)
		return 2;
	if (mode == GL_EXP2)
		return 3;
	return 0;
}

static unsigned int wrapMode(unsigned int wrap)
{
	if (wrap == GL_CLAMP)
		return 1;
	if (wrap == GL_CLAMP_TO_EDGE)
		return 2;
	return 0;
}

static unsigned int colorMaterialFace(unsigned int face)
{
	if (face == GL_FRONT)
		return 1;
	if (face == GL_BACK)
		return 2;
	return 3;
}

static unsigned int colorMaterialMode(unsigned int mode)
{
	if (mode == GL_AMBIENT)
		return 0;
	if (mode == GL_DIFFUSE)
		return 1;
	if (mode == GL_AMBIENT_AND_DIFFUSE)
		return 2;
	if (mode == GL_SPECULAR)
		return 3;
	return 4;
}

static void fillGPUState(const legacygl::ResolvedDraw &command, D3D12GPUState &gpuState)
{
	std::memcpy(gpuState.modelView, command.modelView.m, sizeof(gpuState.modelView));
	std::memcpy(gpuState.projection, command.projection.m, sizeof(gpuState.projection));
	std::memcpy(gpuState.texture, command.textureMatrix.m, sizeof(gpuState.texture));
	std::memcpy(gpuState.normal, command.normal.m, sizeof(gpuState.normal));
	copy4(gpuState.globalAmbient, command.lighting.modelAmbient);
	copyMaterial(gpuState.frontMaterial, command.lighting.frontMaterial);
	copyMaterial(gpuState.backMaterial, command.lighting.backMaterial);

	unsigned int lightMask = 0;
	for (int i = 0; i < 8; i++)
	{
		const legacygl::LightState &source = command.lighting.lights[i];
		D3D12GPULight &destination = gpuState.lights[i];
		copy4(destination.ambient, source.ambient);
		copy4(destination.diffuse, source.diffuse);
		copy4(destination.specular, source.specular);
		copy4(destination.positionEye, source.positionEye);
		destination.spotDirectionCutoff[0] = source.spotDirectionEye[0];
		destination.spotDirectionCutoff[1] = source.spotDirectionEye[1];
		destination.spotDirectionCutoff[2] = source.spotDirectionEye[2];
		destination.spotDirectionCutoff[3] = source.spotCutoff == 180.0f ? -1.0f :
			std::cos(source.spotCutoff * 0.01745329251994329577f);
		destination.attenuationExponent[0] = source.constantAttenuation;
		destination.attenuationExponent[1] = source.linearAttenuation;
		destination.attenuationExponent[2] = source.quadraticAttenuation;
		destination.attenuationExponent[3] = source.spotExponent;
		if (source.enabled)
			lightMask |= 1u << i;
	}

	copy4(gpuState.fogColor, command.fog.color);
	gpuState.fogParams[0] = command.fog.start;
	gpuState.fogParams[1] = command.fog.end;
	gpuState.fogParams[2] = command.fog.density;
	gpuState.fogParams[3] = command.pipeline.alphaReference;
	gpuState.normalParams[0] = command.normalRescaleFactor;
	gpuState.flags0[0] = command.enables.lighting ? 1u : 0u;
	gpuState.flags0[1] = command.enables.texture2D ? 1u : 0u;
	gpuState.flags0[2] = command.enables.colorMaterial ? 1u : 0u;
	gpuState.flags0[3] = command.pipeline.shadeModel == GL_FLAT ? 1u : 0u;
	gpuState.flags1[0] = command.enables.normalize ? 2u :
		(command.enables.rescaleNormal ? 1u : 0u);
	gpuState.flags1[1] = lightMask;
	gpuState.flags1[2] = fogMode(command.fog.mode, command.enables.fog);
	gpuState.flags1[3] = command.fog.distanceMode == GL_EYE_RADIAL_NV ? 2u :
		(command.fog.distanceMode == GL_EYE_PLANE ? 1u : 0u);
	gpuState.flags2[0] = command.enables.alphaTest ? 1u : 0u;
	gpuState.flags2[1] = alphaFunction(command.pipeline.alphaFunction);
	gpuState.flags2[2] = wrapMode(command.texture.wrapS);
	gpuState.flags2[3] = wrapMode(command.texture.wrapT);
	gpuState.flags3[0] = colorMaterialFace(command.lighting.colorMaterialFace);
	gpuState.flags3[1] = colorMaterialMode(command.lighting.colorMaterialMode);
	gpuState.flags3[2] = command.texture.complete ? 1u : 0u;
	gpuState.currentColor[0] = command.currentAttributes.r;
	gpuState.currentColor[1] = command.currentAttributes.g;
	gpuState.currentColor[2] = command.currentAttributes.b;
	gpuState.currentColor[3] = command.currentAttributes.a;
	gpuState.currentNormal[0] = command.currentAttributes.nx;
	gpuState.currentNormal[1] = command.currentAttributes.ny;
	gpuState.currentNormal[2] = command.currentAttributes.nz;
	gpuState.currentTexCoord[0] = command.currentAttributes.s;
	gpuState.currentTexCoord[1] = command.currentAttributes.t;
	gpuState.flags4[0] = command.geometry->hasColor ? 1u : 0u;
	gpuState.flags4[1] = command.geometry->hasNormal ? 1u : 0u;
	gpuState.flags4[2] = command.geometry->hasTexCoord ? 1u : 0u;
}

static D3D12GPUVertex makeGPUVertex(const legacygl::Vertex &vertex,
	const legacygl::Vertex &flat)
{
	D3D12GPUVertex result = {};
	result.position[0] = vertex.x; result.position[1] = vertex.y; result.position[2] = vertex.z;
	result.color[0] = vertex.r; result.color[1] = vertex.g;
	result.color[2] = vertex.b; result.color[3] = vertex.a;
	result.normal[0] = vertex.nx; result.normal[1] = vertex.ny; result.normal[2] = vertex.nz;
	result.texCoord[0] = vertex.s; result.texCoord[1] = vertex.t;
	result.flatPosition[0] = flat.x; result.flatPosition[1] = flat.y; result.flatPosition[2] = flat.z;
	result.flatColor[0] = flat.r; result.flatColor[1] = flat.g;
	result.flatColor[2] = flat.b; result.flatColor[3] = flat.a;
	result.flatNormal[0] = flat.nx; result.flatNormal[1] = flat.ny; result.flatNormal[2] = flat.nz;
	return result;
}

static const legacygl::Vertex &sourceVertex(const legacygl::ResolvedDraw &command,
	std::size_t index)
{
	return command.geometry->vertices[index];
}

static void writeGPUVertices(const legacygl::ResolvedDraw &command, int verticesPerPrimitive,
	void *destination)
{
	unsigned char *write = static_cast<unsigned char *>(destination);
	for (const legacygl::CanonicalPrimitive &primitive : command.primitives->primitives)
	{
		const legacygl::Vertex &flat = sourceVertex(command,
			static_cast<std::size_t>(primitive.provoking));
		for (int i = 0; i < verticesPerPrimitive; i++)
		{
			const legacygl::Vertex &vertex = sourceVertex(command,
				static_cast<std::size_t>(primitive.indices[i]));
			const D3D12GPUVertex gpuVertex = makeGPUVertex(vertex, flat);
			std::memcpy(write, &gpuVertex, sizeof(gpuVertex));
			write += sizeof(gpuVertex);
		}
	}
}


static bool allocateResidentRange(ResidentPage &page, UINT64 size,
	UINT64 alignment, UINT64 &offset)
{
	for (std::size_t i = 0; i < page.freeRanges.size(); i++)
	{
		const ResidentFreeRange range = page.freeRanges[i];
		const UINT64 alignedOffset = alignSize(range.offset, alignment);
		if (alignedOffset < range.offset)
			continue;
		const UINT64 padding = alignedOffset - range.offset;
		if (padding > range.size || size > range.size - padding)
			continue;

		page.freeRanges.erase(page.freeRanges.begin() + static_cast<std::ptrdiff_t>(i));
		std::size_t insertion = i;
		if (padding != 0)
		{
			page.freeRanges.insert(page.freeRanges.begin() + static_cast<std::ptrdiff_t>(insertion),
				{ range.offset, padding });
			insertion++;
		}
		const UINT64 remaining = range.size - padding - size;
		if (remaining != 0)
		{
			page.freeRanges.insert(page.freeRanges.begin() + static_cast<std::ptrdiff_t>(insertion),
				{ alignedOffset + size, remaining });
		}
		offset = alignedOffset;
		return true;
	}
	return false;
}

static void releaseResidentRange(ResidentPage &page, UINT64 offset, UINT64 size)
{
	ResidentFreeRange released = { offset, size };
	auto found = std::lower_bound(page.freeRanges.begin(), page.freeRanges.end(), offset,
		[](const ResidentFreeRange &range, UINT64 value)
		{
			return range.offset < value;
		});
	found = page.freeRanges.insert(found, released);
	if (found != page.freeRanges.begin())
	{
		auto previous = found - 1;
		if (previous->offset + previous->size == found->offset)
		{
			previous->size += found->size;
			found = page.freeRanges.erase(found);
			found = previous;
		}
	}
	auto next = found + 1;
	if (next != page.freeRanges.end() && found->offset + found->size == next->offset)
	{
		found->size += next->size;
		page.freeRanges.erase(next);
	}
}

static void destroyResidentAllocation(ResidentAllocation *allocation)
{
	if (allocation == nullptr)
		return;
	if (allocation->page != nullptr)
		releaseResidentRange(*allocation->page, allocation->offset, allocation->size);
	if (state.residentGeometryBytes >= allocation->size)
		state.residentGeometryBytes -= allocation->size;
	else
		state.residentGeometryBytes = 0;
	delete allocation;
}

static std::shared_ptr<ResidentAllocation> allocateResidentGeometry(UINT64 size,
	UINT64 alignment)
{
	ResidentPage *selectedPage = nullptr;
	UINT64 offset = 0;
	for (const std::unique_ptr<ResidentPage> &page : state.residentPages)
	{
		if (allocateResidentRange(*page, size, alignment, offset))
		{
			selectedPage = page.get();
			break;
		}
	}
	if (selectedPage == nullptr)
	{
		std::unique_ptr<ResidentPage> page(new ResidentPage());
		const UINT64 pageSize = std::max(D3D12_RESIDENT_PAGE_SIZE, alignSize(size, alignment));
		const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_DEFAULT);
		const D3D12_RESOURCE_DESC description = bufferDescription(pageSize);
		requireSuccess(state.device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
			&description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
			IID_PPV_ARGS(&page->resource)), "CreateCommittedResource(resident geometry page)");
		page->freeRanges.push_back({ 0, pageSize });
		selectedPage = page.get();
		state.residentPages.push_back(std::move(page));
		if (!allocateResidentRange(*selectedPage, size, alignment, offset))
			throw std::runtime_error("D3D12 resident geometry page allocation failed");
	}

	ResidentAllocation *allocation = new ResidentAllocation();
	allocation->page = selectedPage;
	allocation->offset = offset;
	allocation->size = size;
	state.residentGeometryBytes += size;
	state.residentGeometryPeakBytes = std::max(state.residentGeometryPeakBytes,
		state.residentGeometryBytes);
	return std::shared_ptr<ResidentAllocation>(allocation, destroyResidentAllocation);
}

static const ResidentGeometryEntry &residentGeometryEntry(
	const legacygl::ResolvedDraw &command, int verticesPerPrimitive,
	UINT vertexCount, UINT64 vertexBytes)
{
	auto found = state.residentGeometry.find(command.geometryResidencyId);
	if (found != state.residentGeometry.end())
	{
		if (found->second.topology != command.primitives->topology ||
			found->second.vertexCount != vertexCount ||
			found->second.hasColor != command.geometry->hasColor ||
			found->second.hasNormal != command.geometry->hasNormal ||
			found->second.hasTexCoord != command.geometry->hasTexCoord)
		{
			throw std::runtime_error("D3D12 resident geometry identity changed while cached");
		}
		state.residentGeometryCacheHits++;
		return found->second;
	}

	state.residentGeometryCacheMisses++;
	ResidentGeometryEntry entry;
	entry.allocation = allocateResidentGeometry(vertexBytes, alignof(D3D12GPUVertex));
	entry.topology = command.primitives->topology;
	entry.vertexCount = vertexCount;
	entry.hasColor = command.geometry->hasColor;
	entry.hasNormal = command.geometry->hasNormal;
	entry.hasTexCoord = command.geometry->hasTexCoord;
	const UploadAllocation upload = allocateUpload(vertexBytes, alignof(D3D12GPUVertex));
	writeGPUVertices(command, verticesPerPrimitive, upload.mapped);
	ResidentPage &page = *entry.allocation->page;
	transitionTrackedResource(page.resource.Get(), page.resourceState,
		D3D12_RESOURCE_STATE_COPY_DEST);
	state.commandList->CopyBufferRegion(page.resource.Get(), entry.allocation->offset, upload.resource,
		upload.offset, vertexBytes);
	transitionTrackedResource(page.resource.Get(), page.resourceState,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	auto inserted = state.residentGeometry.emplace(command.geometryResidencyId, std::move(entry));
	return inserted.first->second;
}

static void releaseResidentGeometry(std::uint64_t residencyId)
{
	state.residentGeometry.erase(residencyId);
}

static D3D12_COMPARISON_FUNC comparisonFunction(unsigned int function)
{
	switch (function)
	{
		case GL_NEVER: return D3D12_COMPARISON_FUNC_NEVER;
		case GL_LESS: return D3D12_COMPARISON_FUNC_LESS;
		case GL_EQUAL: return D3D12_COMPARISON_FUNC_EQUAL;
		case GL_LEQUAL: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case GL_GREATER: return D3D12_COMPARISON_FUNC_GREATER;
		case GL_NOTEQUAL: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case GL_GEQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		default: return D3D12_COMPARISON_FUNC_ALWAYS;
	}
}

static D3D12_BLEND blendFactor(unsigned int factor)
{
	switch (factor)
	{
		case GL_ZERO: return D3D12_BLEND_ZERO;
		case GL_ONE: return D3D12_BLEND_ONE;
		case GL_SRC_COLOR: return D3D12_BLEND_SRC_COLOR;
		case GL_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_COLOR;
		case GL_DST_COLOR: return D3D12_BLEND_DEST_COLOR;
		case GL_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_COLOR;
		case GL_SRC_ALPHA: return D3D12_BLEND_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_ALPHA: return D3D12_BLEND_INV_SRC_ALPHA;
		case GL_DST_ALPHA: return D3D12_BLEND_DEST_ALPHA;
		case GL_ONE_MINUS_DST_ALPHA: return D3D12_BLEND_INV_DEST_ALPHA;
		case GL_SRC_ALPHA_SATURATE: return D3D12_BLEND_SRC_ALPHA_SAT;
		default: return D3D12_BLEND_ONE;
	}
}

static D3D12_BLEND alphaBlendFactor(unsigned int factor)
{
	switch (factor)
	{
		case GL_SRC_COLOR: return D3D12_BLEND_SRC_ALPHA;
		case GL_ONE_MINUS_SRC_COLOR: return D3D12_BLEND_INV_SRC_ALPHA;
		case GL_DST_COLOR: return D3D12_BLEND_DEST_ALPHA;
		case GL_ONE_MINUS_DST_COLOR: return D3D12_BLEND_INV_DEST_ALPHA;
		case GL_SRC_ALPHA_SATURATE: return D3D12_BLEND_ONE;
		default: return blendFactor(factor);
	}
}

static D3D12_LOGIC_OP logicOperation(unsigned int operation)
{
	switch (operation)
	{
		case GL_CLEAR: return D3D12_LOGIC_OP_CLEAR;
		case GL_AND: return D3D12_LOGIC_OP_AND;
		case GL_AND_REVERSE: return D3D12_LOGIC_OP_AND_REVERSE;
		case GL_COPY: return D3D12_LOGIC_OP_COPY;
		case GL_AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
		case GL_NOOP: return D3D12_LOGIC_OP_NOOP;
		case GL_XOR: return D3D12_LOGIC_OP_XOR;
		case GL_OR: return D3D12_LOGIC_OP_OR;
		case GL_NOR: return D3D12_LOGIC_OP_NOR;
		case GL_EQUIV: return D3D12_LOGIC_OP_EQUIV;
		case GL_INVERT: return D3D12_LOGIC_OP_INVERT;
		case GL_OR_REVERSE: return D3D12_LOGIC_OP_OR_REVERSE;
		case GL_COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
		case GL_OR_INVERTED: return D3D12_LOGIC_OP_OR_INVERTED;
		case GL_NAND: return D3D12_LOGIC_OP_NAND;
		default: return D3D12_LOGIC_OP_SET;
	}
}

static D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType(legacygl::Topology topology)
{
	if (topology == legacygl::Topology::Points)
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	if (topology == legacygl::Topology::Lines)
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

static D3D_PRIMITIVE_TOPOLOGY primitiveTopology(legacygl::Topology topology)
{
	if (topology == legacygl::Topology::Points)
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	if (topology == legacygl::Topology::Lines)
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

static float bitsFloat(std::uint32_t bits)
{
	float value = 0.0f;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

static INT depthBiasValue(float units)
{
	// GL units are floating-point multiples of an implementation's minimum
	// resolvable depth difference. D3D's UNORM DepthBias is an integer count of
	// that difference, so preserve every nonzero GL offset with the smallest
	// representable D3D bias instead of rounding fractional units to zero.
	const double value = static_cast<double>(units);
	const double rounded = value < 0.0 ? std::floor(value) : std::ceil(value);
	if (rounded > static_cast<double>(std::numeric_limits<INT>::max()))
		return std::numeric_limits<INT>::max();
	if (rounded < static_cast<double>(std::numeric_limits<INT>::min()))
		return std::numeric_limits<INT>::min();
	return static_cast<INT>(rounded);
}

static ComPtr<ID3D12PipelineState> createLegacyPipeline(const PipelineKey &key)
{
	D3D12_INPUT_ELEMENT_DESC input[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, position)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, color)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, normal)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, texCoord)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, flatPosition)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, flatColor)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 1, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			static_cast<UINT>(offsetof(D3D12GPUVertex, flatNormal)), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
	description.pRootSignature = state.legacyRootSignature.Get();
	description.VS = shaderBytecode(state.legacyVertexShader.Get());
	description.PS = shaderBytecode(key.logicOp ? state.legacyLogicPixelShader.Get() :
		state.legacyPixelShader.Get());
	description.BlendState = defaultBlend();
	D3D12_RENDER_TARGET_BLEND_DESC &blend = description.BlendState.RenderTarget[0];
	blend.BlendEnable = key.blend && !key.logicOp ? TRUE : FALSE;
	blend.LogicOpEnable = key.logicOp ? TRUE : FALSE;
	blend.SrcBlend = blendFactor(key.blendSource);
	blend.DestBlend = blendFactor(key.blendDestination);
	blend.SrcBlendAlpha = alphaBlendFactor(key.blendSource);
	blend.DestBlendAlpha = alphaBlendFactor(key.blendDestination);
	blend.LogicOp = logicOperation(key.logicOpcode);
	blend.RenderTargetWriteMask = static_cast<UINT8>(key.colorWriteMask);
	description.SampleMask = UINT_MAX;
	description.RasterizerState = defaultRasterizer();
	if (key.cullFace)
	{
		description.RasterizerState.CullMode = key.cullFaceMode == GL_FRONT ?
			D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK;
	}
	// The asymmetric culling fixture establishes that D3D12's facing test
	// preserves the winding of the corrected GL clip coordinates.
	description.RasterizerState.FrontCounterClockwise =
		key.frontFaceMode == GL_CCW ? TRUE : FALSE;
	if (key.depthBias)
	{
		description.RasterizerState.DepthBias = depthBiasValue(bitsFloat(key.polygonOffsetUnits));
		description.RasterizerState.SlopeScaledDepthBias = bitsFloat(key.polygonOffsetFactor);
	}
	description.DepthStencilState = disabledDepthStencil();
	description.DepthStencilState.DepthEnable = key.depthTest ? TRUE : FALSE;
	description.DepthStencilState.DepthWriteMask = key.depthWrite ?
		D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	description.DepthStencilState.DepthFunc = comparisonFunction(key.depthFunction);
	description.DepthStencilState.StencilEnable = key.stencilTest ? TRUE : FALSE;
	description.InputLayout = { input, static_cast<UINT>(std::size(input)) };
	description.PrimitiveTopologyType = topologyType(key.topology);
	description.NumRenderTargets = 1;
	description.RTVFormats[0] = key.logicOp ? D3D12_LOGIC_COLOR_FORMAT : D3D12_COLOR_FORMAT;
	description.DSVFormat = D3D12_DEPTH_FORMAT;
	description.SampleDesc.Count = 1;
	ComPtr<ID3D12PipelineState> pipeline;
	requireSuccess(state.device->CreateGraphicsPipelineState(&description,
		IID_PPV_ARGS(&pipeline)), "CreateGraphicsPipelineState(legacy)");
	return pipeline;
}

static PipelineKey pipelineKey(const legacygl::ResolvedDraw &command)
{
	PipelineKey key;
	key.topology = command.primitives->topology;
	key.depthTest = command.enables.depthTest;
	key.depthWrite = command.pipeline.depthWrite;
	key.depthFunction = command.pipeline.depthFunction;
	key.cullFace = command.enables.cullFace;
	key.cullFaceMode = command.pipeline.cullFaceMode;
	key.frontFaceMode = command.pipeline.frontFaceMode;
	key.blend = command.enables.blend;
	key.blendSource = command.pipeline.blendSource;
	key.blendDestination = command.pipeline.blendDestination;
	key.logicOp = command.enables.colorLogicOp;
	key.logicOpcode = command.pipeline.logicOpcode;
	for (int i = 0; i < 4; i++)
	{
		if (command.pipeline.colorWrite[i])
			key.colorWriteMask |= 1u << i;
	}
	key.stencilTest = command.enables.stencilTest;
	key.depthBias = command.enables.polygonOffsetFill &&
		command.primitives->topology == legacygl::Topology::Triangles;
	key.polygonOffsetFactor = floatBits(command.pipeline.polygonOffsetFactor);
	key.polygonOffsetUnits = floatBits(command.pipeline.polygonOffsetUnits);
	return key;
}

static ID3D12PipelineState *legacyPipeline(const PipelineKey &key)
{
	auto found = state.pipelines.find(key);
	if (found != state.pipelines.end())
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.legacyPipelineMapHits++;
#endif
		return found->second.Get();
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	const std::chrono::steady_clock::time_point start = state.diagnostics.enabled ?
		std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point();
#endif
	ComPtr<ID3D12PipelineState> pipeline = createLegacyPipeline(key);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
	{
		state.diagnostics.legacyPipelineCreates++;
		state.diagnostics.legacyPipelineCreationNanoseconds += static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(
				std::chrono::steady_clock::now() - start).count());
	}
#endif
	auto inserted = state.pipelines.insert(std::make_pair(key, pipeline));
	return inserted.first->second.Get();
}

static void bindLegacyPipeline(const legacygl::ResolvedDraw &command)
{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.legacyPipelineRequests++;
#endif
	const PipelineKey key = pipelineKey(command);
	if (state.commandListState.legacyPipelineValid &&
		state.commandListState.legacyPipelineKey == key)
	{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		if (state.diagnostics.enabled)
			state.diagnostics.legacyPipelineCurrentHits++;
#endif
		return;
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.legacyPipelineMapLookups++;
#endif
	state.commandList->SetPipelineState(legacyPipeline(key));
	state.commandListState.legacyPipelineKey = key;
	state.commandListState.legacyPipelineValid = true;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (state.diagnostics.enabled)
		state.diagnostics.legacyPipelineActualBinds++;
#endif
}

static unsigned int colorWriteMask(const bool colorWrite[4])
{
	unsigned int mask = 0;
	if (colorWrite[0]) mask |= D3D12_COLOR_WRITE_ENABLE_RED;
	if (colorWrite[1]) mask |= D3D12_COLOR_WRITE_ENABLE_GREEN;
	if (colorWrite[2]) mask |= D3D12_COLOR_WRITE_ENABLE_BLUE;
	if (colorWrite[3]) mask |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
	return mask;
}

static ID3D12PipelineState *clearPipeline(unsigned int writeMask)
{
	if (writeMask >= state.clearPipelines.size())
		throw std::runtime_error("D3D12 clear color-write mask is invalid");
	if (state.clearPipelines[writeMask] == nullptr)
	{
		state.clearPipelines[writeMask] = createSimplePipeline(state.clearVertexShader.Get(),
			state.clearPixelShader.Get(), writeMask);
	}
	return state.clearPipelines[writeMask].Get();
}

static void recordPartialColorClear(const legacygl::ResolvedClear &command,
	unsigned int writeMask)
{
	invalidateCommandListState();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeRenderTargetPassEmulationBegin(RenderTargetPassEmulation::PartialClear);
#endif
	bindOffscreenTargets(false);
	const D3D12_VIEWPORT viewport = fullViewport();
	const D3D12_RECT scissor = fullScissor();
	state.commandList->RSSetViewports(1, &viewport);
	state.commandList->RSSetScissorRects(1, &scissor);
	state.commandList->SetGraphicsRootSignature(state.simpleRootSignature.Get());
	state.commandList->SetPipelineState(clearPipeline(writeMask));
	state.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	state.commandList->SetGraphicsRoot32BitConstants(0, 4, command.color, 0);
	state.commandList->DrawInstanced(3, 1, 0, 0);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeRenderTargetPassEmulationEnd();
#endif
}

static void recordPresent()
{
	invalidateCommandListState();
	transitionTrackedResource(state.colorTarget.Get(), state.colorTargetState,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	const UINT index = state.swapChain->GetCurrentBackBufferIndex();
	transitionResource(state.backBuffers[index].Get(), D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeRenderTargetPassEmulationBegin(RenderTargetPassEmulation::Present);
#endif
	const D3D12_CPU_DESCRIPTOR_HANDLE target = cpuDescriptor(state.rtvHeap.Get(), index + 1,
		state.rtvDescriptorSize);
	state.commandList->OMSetRenderTargets(1, &target, FALSE, nullptr);
	const D3D12_VIEWPORT viewport = fullViewport();
	const D3D12_RECT scissor = fullScissor();
	state.commandList->RSSetViewports(1, &viewport);
	state.commandList->RSSetScissorRects(1, &scissor);
	ID3D12DescriptorHeap *heaps[] = { state.srvHeap.Get() };
	state.commandList->SetDescriptorHeaps(1, heaps);
	state.commandList->SetGraphicsRootSignature(state.simpleRootSignature.Get());
	state.commandList->SetPipelineState(state.presentPipeline.Get());
	state.commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	state.commandList->SetGraphicsRootDescriptorTable(1,
		gpuDescriptor(state.srvHeap.Get(), 1, state.srvDescriptorSize));
	state.commandList->DrawInstanced(3, 1, 0, 0);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	observeRenderTargetPassEmulationEnd();
#endif

	transitionResource(state.backBuffers[index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);
	transitionTrackedResource(state.colorTarget.Get(), state.colorTargetState,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
}

static void submitFrame(bool display)
{
	closeAndExecute();
	if (display)
	{
		const UINT flags = state.allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0;
		requireSuccess(state.swapChain->Present(0, flags), "Present");
	}
	signalCurrentFrame();
	requireNoDebugErrors();
	state.frameSlot = (state.frameSlot + 1) % D3D12_FRAMES_IN_FLIGHT;
	beginFrame();
}

static void readBackColor(const legacygl::ResolvedReadback &command)
{
	if (command.type != GL_UNSIGNED_BYTE)
		throw std::runtime_error("D3D12 readback only accepts GL_UNSIGNED_BYTE");
	const legacygl::PixelTransferFormat *transfer =
		legacygl::unsignedBytePixelTransferFormat(command.format);
	if (transfer == nullptr)
		throw std::runtime_error("D3D12 readback received an unsupported pixel format");
	const int components = transfer->components;
	if (command.x < 0 || command.y < 0 || command.x + command.width > state.targetWidth ||
		command.y + command.height > state.targetHeight)
	{
		throw std::runtime_error("D3D12 readback rectangle exceeds the render target");
	}

	const UINT rowPitch = static_cast<UINT>(alignSize(
		static_cast<UINT64>(command.width) * 4, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
	const UINT64 byteSize = static_cast<UINT64>(rowPitch) *
		static_cast<UINT64>(command.height);
	const D3D12_HEAP_PROPERTIES properties = heapProperties(D3D12_HEAP_TYPE_READBACK);
	const D3D12_RESOURCE_DESC description = bufferDescription(byteSize);
	ComPtr<ID3D12Resource> readback;
	requireSuccess(state.device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
		&description, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)),
		"CreateCommittedResource(readback)");

	transitionTrackedResource(state.colorTarget.Get(), state.colorTargetState,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	D3D12_TEXTURE_COPY_LOCATION destination = {};
	destination.pResource = readback.Get();
	destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	destination.PlacedFootprint.Footprint.Format = D3D12_COLOR_FORMAT;
	destination.PlacedFootprint.Footprint.Width = static_cast<UINT>(command.width);
	destination.PlacedFootprint.Footprint.Height = static_cast<UINT>(command.height);
	destination.PlacedFootprint.Footprint.Depth = 1;
	destination.PlacedFootprint.Footprint.RowPitch = rowPitch;
	D3D12_TEXTURE_COPY_LOCATION source = {};
	source.pResource = state.colorTarget.Get();
	source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	D3D12_BOX sourceBox = {};
	sourceBox.left = static_cast<UINT>(command.x);
	sourceBox.top = static_cast<UINT>(state.targetHeight - (command.y + command.height));
	sourceBox.front = 0;
	sourceBox.right = static_cast<UINT>(command.x + command.width);
	sourceBox.bottom = static_cast<UINT>(state.targetHeight - command.y);
	sourceBox.back = 1;
	state.commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, &sourceBox);
	transitionTrackedResource(state.colorTarget.Get(), state.colorTargetState,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	flushAndWait();

	void *mapped = nullptr;
	D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(byteSize) };
	requireSuccess(readback->Map(0, &readRange, &mapped), "Map(readback)");
	const std::size_t destinationStride = alignedRowSize(
		static_cast<std::size_t>(command.width) * static_cast<std::size_t>(components),
		command.packAlignment);
	unsigned char *destinationPixels = static_cast<unsigned char *>(command.pixels);
	const unsigned char *sourcePixels = static_cast<const unsigned char *>(mapped);
	for (int y = 0; y < command.height; y++)
	{
		const unsigned char *sourceRow = sourcePixels +
			static_cast<std::size_t>(command.height - 1 - y) * rowPitch;
		unsigned char *destinationRow = destinationPixels +
			static_cast<std::size_t>(y) * destinationStride;
		for (int x = 0; x < command.width; x++)
		{
			if (!legacygl::encodeUnsignedBytePixel(
				sourceRow + static_cast<std::size_t>(x) * 4, command.format,
				destinationRow + static_cast<std::size_t>(x) *
					static_cast<std::size_t>(components)))
			{
				throw std::runtime_error("D3D12 readback conversion failed");
			}
		}
	}
	D3D12_RANGE writeRange = { 0, 0 };
	readback->Unmap(0, &writeRange);
}

static void releaseStateResources()
{
	state.residentGeometry.clear();
	for (FrameResources &frame : state.frames)
	{
		frame.residentAllocations.clear();
		for (UploadChunk &chunk : frame.uploadChunks)
		{
			if (chunk.resource != nullptr && chunk.mapped != nullptr)
				chunk.resource->Unmap(0, nullptr);
			chunk.mapped = nullptr;
		}
	}
	state.residentPages.clear();
	if (state.fenceEvent != nullptr)
		CloseHandle(state.fenceEvent);
	state = State();
}

#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
static const char *pipelineTopologyName(legacygl::Topology topology)
{
	switch (topology)
	{
	case legacygl::Topology::Points: return "points";
	case legacygl::Topology::Lines: return "lines";
	case legacygl::Topology::Triangles: return "triangles";
	default: return "unknown";
	}
}

static void dumpPipelineKeys()
{
	if (state.pipelineKeyDumpPath.empty())
		return;
	std::ofstream output(state.pipelineKeyDumpPath.c_str(),
		std::ios::out | std::ios::binary | std::ios::trunc);
	if (!output)
	{
		std::fprintf(stderr, "D3D12 pipeline-key dump could not open %s\n",
			state.pipelineKeyDumpPath.c_str());
		return;
	}
	output.imbue(std::locale::classic());
	output << "d3d12_pipeline_keys_v1\n";
	output << "count=" << state.pipelines.size() << '\n';
	std::size_t index = 0;
	for (const auto &entry : state.pipelines)
	{
		const PipelineKey &key = entry.first;
		output << "key[" << index++ << "]" <<
			" topology=" << pipelineTopologyName(key.topology) <<
			" depth_test=" << (key.depthTest ? 1 : 0) <<
			" depth_write=" << (key.depthWrite ? 1 : 0) <<
			" depth_function=" << key.depthFunction <<
			" cull_face=" << (key.cullFace ? 1 : 0) <<
			" cull_face_mode=" << key.cullFaceMode <<
			" front_face_mode=" << key.frontFaceMode <<
			" blend=" << (key.blend ? 1 : 0) <<
			" blend_source=" << key.blendSource <<
			" blend_destination=" << key.blendDestination <<
			" logic_op=" << (key.logicOp ? 1 : 0) <<
			" logic_opcode=" << key.logicOpcode <<
			" color_write_mask=" << key.colorWriteMask <<
			" render_target_format=" <<
				(key.logicOp ? D3D12_LOGIC_COLOR_FORMAT : D3D12_COLOR_FORMAT) <<
			" depth_stencil_format=" << D3D12_DEPTH_FORMAT <<
			" sample_count=1" <<
			" sample_quality=0" <<
			" stencil_test=" << (key.stencilTest ? 1 : 0) <<
			" depth_bias=" << (key.depthBias ? 1 : 0) <<
			" polygon_offset_factor_bits=0x" << std::hex << std::nouppercase << std::setw(8) <<
				std::setfill('0') << key.polygonOffsetFactor <<
			" polygon_offset_units_bits=0x" << std::setw(8) << key.polygonOffsetUnits <<
			std::dec << std::setfill(' ') << '\n';
	}
	if (!output)
	{
		std::fprintf(stderr, "D3D12 pipeline-key dump failed while writing %s\n",
			state.pipelineKeyDumpPath.c_str());
	}
}

static void printDiagnostics()
{
	if (!state.diagnostics.enabled)
		return;
	const Diagnostics &diagnostics = state.diagnostics;
	std::cout << "d3d12 diagnostics: legacy_pipeline_requests=" <<
		diagnostics.legacyPipelineRequests <<
		" legacy_pipeline_current_hits=" << diagnostics.legacyPipelineCurrentHits <<
		" legacy_pipeline_map_lookups=" << diagnostics.legacyPipelineMapLookups <<
		" legacy_pipeline_map_hits=" << diagnostics.legacyPipelineMapHits <<
		" legacy_pipeline_creates=" << diagnostics.legacyPipelineCreates <<
		" legacy_pipeline_creation_ns=" << diagnostics.legacyPipelineCreationNanoseconds <<
		" legacy_pipeline_unique_keys=" << state.pipelines.size() <<
		" legacy_pipeline_actual_binds=" << diagnostics.legacyPipelineActualBinds << '\n';
	std::cout << "d3d12 diagnostics: legacy_root_requests=" << diagnostics.rootSignatureRequests <<
		" legacy_root_current_hits=" << diagnostics.rootSignatureCurrentHits <<
		" legacy_root_actual_binds=" << diagnostics.rootSignatureActualBinds <<
		" legacy_topology_requests=" << diagnostics.topologyRequests <<
		" legacy_topology_current_hits=" << diagnostics.topologyCurrentHits <<
		" legacy_topology_actual_binds=" << diagnostics.topologyActualBinds <<
		" legacy_viewport_requests=" << diagnostics.viewportRequests <<
		" legacy_viewport_current_hits=" << diagnostics.viewportCurrentHits <<
		" legacy_viewport_actual_sets=" << diagnostics.viewportActualSets <<
		" legacy_scissor_requests=" << diagnostics.scissorRequests <<
		" legacy_scissor_current_hits=" << diagnostics.scissorCurrentHits <<
		" legacy_scissor_actual_sets=" << diagnostics.scissorActualSets << '\n';
	std::cout << "d3d12 diagnostics: sampler_requests=" << diagnostics.samplerRequests <<
		" sampler_current_hits=" << diagnostics.samplerCurrentHits <<
		" sampler_map_lookups=" << diagnostics.samplerMapLookups <<
		" sampler_map_hits=" << diagnostics.samplerMapHits <<
		" sampler_allocations=" << diagnostics.samplerAllocations <<
		" sampler_table_requests=" << diagnostics.samplerTableRequests <<
		" sampler_table_current_hits=" << diagnostics.samplerTableCurrentHits <<
		" sampler_table_actual_binds=" << diagnostics.samplerTableActualBinds <<
		" srv_allocation_lookups=" << diagnostics.srvAllocationLookups <<
		" srv_free_list_hits=" << diagnostics.srvFreeListHits <<
		" srv_allocations=" << diagnostics.srvAllocations <<
		" srv_fresh_allocations=" << diagnostics.srvFreshAllocations <<
		" srv_invalidations_retired=" << diagnostics.srvInvalidationsRetired <<
		" srv_reclaimed=" << diagnostics.srvDescriptorsReclaimed << '\n';
	std::cout << "d3d12 diagnostics: retained_resource_requests=" <<
		diagnostics.retainedResourceRequests <<
		" retained_resource_duplicates=" << diagnostics.retainedResourceDuplicates <<
		" retained_resource_references=" << diagnostics.retainedResourceReferences <<
		" reclaimed_resource_references=" << diagnostics.reclaimedResourceReferences <<
		" retained_resident_allocation_references=" <<
		diagnostics.retainedResidentAllocationReferences <<
		" reclaimed_resident_allocation_references=" <<
		diagnostics.reclaimedResidentAllocationReferences <<
		" transitions_emitted=" << diagnostics.transitionsEmitted <<
		" transitions_skipped=" << diagnostics.transitionsSkipped << '\n';
	std::cout << "d3d12 diagnostics: native_render_pass_begins=0" <<
		" native_render_pass_ends=0" <<
		" render_target_pass_emulation_begins=" <<
		diagnostics.renderTargetPassEmulationBegins <<
		" render_target_pass_emulation_ends=" << diagnostics.renderTargetPassEmulationEnds <<
		" offscreen_pass_emulations=" << diagnostics.offscreenPassEmulations <<
		" partial_clear_pass_emulations=" << diagnostics.partialClearPassEmulations <<
		" present_pass_emulations=" << diagnostics.presentPassEmulations << '\n';
	std::cout << "d3d12 diagnostics: fence_wait_checks=" << diagnostics.fenceWaitChecks <<
		" fence_waits=" << diagnostics.fenceWaits <<
		" fence_wait_ns=" << diagnostics.fenceWaitNanoseconds <<
		" flush_drains=" << diagnostics.flushDrains <<
		" finish_drains=" << diagnostics.finishDrains << '\n';
}

static void configureDiagnostics()
{
	const char *diagnostics = std::getenv("A126_RENDER_DIAGNOSTICS");
	state.diagnostics.enabled = diagnostics != nullptr && std::strcmp(diagnostics, "1") == 0;
	const char *pipelineKeyDump = std::getenv("A126_PIPELINE_KEY_DUMP");
	if (pipelineKeyDump != nullptr && pipelineKeyDump[0] != '\0')
		state.pipelineKeyDumpPath = pipelineKeyDump;
}
#endif

static void initialize()
{
	if (state.initialized)
		return;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	configureDiagnostics();
#endif
	state.residentGeometry.reserve(8192);
	for (FrameResources &frame : state.frames)
		frame.retainedResourceSet.reserve(32);
	try
	{
		platform::createWindow(platform::WindowGraphicsAPI::Direct3D);
		createDevice();
		createDescriptorHeaps();
		createRootSignatures();
		createShadersAndPipelines();
		createSwapChain();
		beginFrame();
		createFallbackTexture();
		state.initialized = true;
		std::cout << "legacygl: selected backend d3d12\n";
	}
	catch (...)
	{
		releaseStateResources();
		platform::destroyWindow();
		throw;
	}
}

static void present()
{
	if (!state.initialized)
		return;
	if (!ensureRenderTargets())
	{
		submitFrame(false);
		return;
	}
	recordPresent();
	submitFrame(true);
}

static void shutdown()
{
	if (!state.initialized && state.device == nullptr)
		return;
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	bool queueDrained = false;
#endif
	if (state.commandQueue != nullptr && state.fence != nullptr)
	{
		if (state.commandListOpen)
		{
			try
			{
				closeAndExecute();
			}
			catch (const std::exception &error)
			{
				std::fprintf(stderr, "D3D12 shutdown command submission failed: %s\n", error.what());
			}
		}
		try
		{
			const UINT64 value = signalCurrentFrame();
			waitForFence(value);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			queueDrained = true;
#endif
		}
		catch (const std::exception &error)
		{
			std::fprintf(stderr, "D3D12 shutdown queue wait failed: %s\n", error.what());
		}
	}
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	if (queueDrained && state.diagnostics.enabled)
	{
		for (const FrameResources &frame : state.frames)
		{
			state.diagnostics.srvDescriptorsReclaimed += frame.retiredSrvDescriptors.size();
			state.diagnostics.reclaimedResourceReferences += frame.retainedResources.size();
			state.diagnostics.reclaimedResidentAllocationReferences +=
				frame.residentAllocations.size();
		}
	}
#endif
	collectDebugMessages();
	const std::uint64_t errors = state.validationErrors;
	const std::uint64_t warnings = state.validationWarnings;
	const std::uint64_t residentCacheHits = state.residentGeometryCacheHits;
	const std::uint64_t residentCacheMisses = state.residentGeometryCacheMisses;
	const UINT64 residentBytes = state.residentGeometryPeakBytes;
	const std::size_t residentPages = state.residentPages.size();
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
	dumpPipelineKeys();
	printDiagnostics();
#endif
	releaseStateResources();
	std::cout << "d3d12: shutdown, validation errors=" << errors <<
		", warnings=" << warnings <<
		", resident cache hits=" << residentCacheHits <<
		", misses=" << residentCacheMisses <<
		", resident bytes=" << residentBytes <<
		", pages=" << residentPages << '\n';
}

class D3D12Sink final : public legacygl::Sink
{
public:
	void matrixMode(unsigned int) override {}
	void loadIdentity() override {}
	void pushMatrix() override {}
	void popMatrix() override {}
	void translatef(float, float, float) override {}
	void rotatef(float, float, float, float) override {}
	void scalef(float, float, float) override {}
	void scaled(double, double, double) override {}
	void ortho(double, double, double, double, double, double) override {}
	void frustum(double, double, double, double, double, double) override {}
	void enable(unsigned int) override {}
	void disable(unsigned int) override {}
	void blendFunc(unsigned int, unsigned int) override {}
	void alphaFunc(unsigned int, float) override {}
	void depthFunc(unsigned int) override {}
	void depthMask(unsigned char) override {}
	void colorMask(unsigned char, unsigned char, unsigned char, unsigned char) override {}
	void cullFace(unsigned int) override {}
	void shadeModel(unsigned int) override {}
	void logicOp(unsigned int) override {}
	void lineWidth(float) override {}
	void polygonOffset(float, float) override {}
	void viewport(int, int, int, int) override {}
	void pixelStorei(unsigned int, int) override {}
	void color4f(float, float, float, float) override {}
	void color3f(float, float, float) override {}
	void normal3f(float, float, float) override {}
	void normal3b(signed char, signed char, signed char) override {}
	void fogf(unsigned int, float) override {}
	void fogfv(unsigned int, const float *) override {}
	void fogi(unsigned int, int) override {}
	void lightfv(unsigned int, unsigned int, const float *) override {}
	void lightModelfv(unsigned int, const float *) override {}
	void colorMaterial(unsigned int, unsigned int) override {}

	void genTextures(int n, unsigned int *textures) override
	{
		if (n <= 0 || textures == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			textures[i] = textureNames.allocate();
			if (textures[i] == 0)
				throw std::runtime_error("D3D12 logical texture-name namespace exhausted");
		}
	}

	void deleteTextures(int n, const unsigned int *textures) override
	{
		if (n <= 0 || textures == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			const unsigned int name = textures[i];
			if (name == 0)
				continue;
			textureNames.release(name);
			auto found = state.textures.find(name);
			if (found != state.textures.end())
			{
				if (state.initialized && found->second.storage.resource != nullptr)
				{
					retainResource(found->second.storage.resource);
					retireSrvDescriptor(found->second.storage.srvIndex);
				}
				state.textures.erase(found);
			}
		}
	}

	void bindTexture(unsigned int, unsigned int name) override { textureNames.reserve(name); }
	void texParameteri(unsigned int, unsigned int, int) override {}
	void texImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int,
		const void *) override {}
	void texSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int,
		const void *) override {}
	void enableClientState(unsigned int) override {}
	void disableClientState(unsigned int) override {}
	void vertexPointer(int, unsigned int, int, const void *) override {}
	void texCoordPointer(int, unsigned int, int, const void *) override {}
	void colorPointer(int, unsigned int, int, const void *) override {}
	void normalPointer(unsigned int, int, const void *) override {}
	void drawArrays(unsigned int, int, int) override {}
	void begin(unsigned int) override {}
	void end() override {}
	void vertex3f(float, float, float) override {}
	void texCoord2f(float, float) override {}

	void genBuffersARB(int n, unsigned int *buffers) override
	{
		if (n <= 0 || buffers == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			buffers[i] = bufferNames.allocate();
			if (buffers[i] == 0)
				throw std::runtime_error("D3D12 logical buffer-name namespace exhausted");
		}
	}

	void bindBufferARB(unsigned int, unsigned int name) override { bufferNames.reserve(name); }
	void bufferDataARB(unsigned int, std::ptrdiff_t, const void *, unsigned int) override {}

	unsigned int genLists(int range) override
	{
		if (range <= 0)
			return 0;
		const std::uint64_t count = static_cast<std::uint64_t>(range);
		const std::uint64_t maximumName = std::numeric_limits<unsigned int>::max();
		if (nextListName > maximumName || count > maximumName - nextListName + 1)
			return 0;
		const unsigned int base = static_cast<unsigned int>(nextListName);
		nextListName += count;
		return base;
	}

	void newList(unsigned int list, unsigned int) override
	{
		if (list != 0)
			nextListName = std::max(nextListName, static_cast<std::uint64_t>(list) + 1);
	}
	void endList() override {}
	void callList(unsigned int) override {}
	void callLists(int, unsigned int, const void *) override {}
	void deleteLists(unsigned int, int) override {}
	void clear(unsigned int) override {}
	void clearColor(float, float, float, float) override {}
	void clearDepth(double) override {}
	void readPixels(int, int, int, int, unsigned int, unsigned int, void *) override {}
	void finish() override
	{
		if (state.initialized)
		{
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
			if (state.diagnostics.enabled)
				state.diagnostics.finishDrains++;
#endif
			flushAndWait();
		}
	}

	bool wantsCanonicalGeometry() const override
	{
		return true;
	}

	void releaseCanonicalGeometry(std::uint64_t residencyId) override
	{
		releaseResidentGeometry(residencyId);
	}

	void resolvedClear(const legacygl::ResolvedClear &command) override
	{
		if (!ensureRenderTargets())
			return;
		const unsigned int writeMask = colorWriteMask(command.colorWrite);
		if ((command.mask & GL_COLOR_BUFFER_BIT) != 0)
		{
			if (writeMask == D3D12_COLOR_WRITE_ENABLE_ALL)
			{
				state.commandList->ClearRenderTargetView(cpuDescriptor(state.rtvHeap.Get(), 0,
					state.rtvDescriptorSize), command.color, 0, nullptr);
			}
			else if (writeMask != 0)
			{
				recordPartialColorClear(command, writeMask);
			}
		}

		unsigned int depthStencilFlags = 0;
		if ((command.mask & GL_DEPTH_BUFFER_BIT) != 0 && command.depthWrite)
			depthStencilFlags |= D3D12_CLEAR_FLAG_DEPTH;
		if ((command.mask & GL_STENCIL_BUFFER_BIT) != 0)
			depthStencilFlags |= D3D12_CLEAR_FLAG_STENCIL;
		if (depthStencilFlags != 0)
		{
			const float depth = static_cast<float>(std::max(0.0, std::min(1.0, command.depth)));
			state.commandList->ClearDepthStencilView(
				state.dsvHeap->GetCPUDescriptorHandleForHeapStart(),
				static_cast<D3D12_CLEAR_FLAGS>(depthStencilFlags), depth, 0, 0, nullptr);
		}
	}

	void resolvedDraw(const legacygl::ResolvedDraw &command) override
	{
		if (!ensureRenderTargets() || command.geometry == nullptr || command.primitives == nullptr ||
			command.geometry->vertices.empty())
		{
			return;
		}
		if (command.enables.lineSmooth)
			throw std::runtime_error("D3D12 backend does not emulate exercised GL_LINE_SMOOTH");
		if (command.enables.cullFace && command.pipeline.cullFaceMode == GL_FRONT_AND_BACK &&
			command.primitives->topology == legacygl::Topology::Triangles)
		{
			return;
		}

		const int verticesPerPrimitive = command.primitives->topology == legacygl::Topology::Points ? 1 :
			(command.primitives->topology == legacygl::Topology::Lines ? 2 : 3);
		const std::size_t vertexCount = command.primitives->primitives.size() *
			static_cast<std::size_t>(verticesPerPrimitive);
		if (vertexCount == 0)
			return;
		if (vertexCount > std::numeric_limits<UINT>::max() ||
			vertexCount > std::numeric_limits<UINT>::max() / sizeof(D3D12GPUVertex))
		{
			throw std::runtime_error("D3D12 geometry exceeds the draw-count range");
		}
		const UINT drawVertexCount = static_cast<UINT>(vertexCount);
		const UINT vertexBytes = static_cast<UINT>(vertexCount * sizeof(D3D12GPUVertex));

		D3D12GPUState gpuState = {};
		fillGPUState(command, gpuState);
		const TextureBinding texture = bindTextureState(command, gpuState);
		retainResource(texture.resource);

		D3D12_VERTEX_BUFFER_VIEW vertexView = {};
		if (command.geometryResidencyId == 0)
		{
			const UploadAllocation upload = allocateUpload(vertexBytes, alignof(D3D12GPUVertex));
			writeGPUVertices(command, verticesPerPrimitive, upload.mapped);
			vertexView.BufferLocation = upload.gpuAddress;
		}
		else
		{
			const ResidentGeometryEntry &entry = residentGeometryEntry(command,
				verticesPerPrimitive, drawVertexCount, vertexBytes);
			retainResidentAllocation(entry.allocation);
			vertexView.BufferLocation = entry.allocation->page->resource->GetGPUVirtualAddress() +
				entry.allocation->offset;
		}
		vertexView.SizeInBytes = vertexBytes;
		vertexView.StrideInBytes = sizeof(D3D12GPUVertex);

		const UploadAllocation constants = allocateUpload(sizeof(gpuState),
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		std::memcpy(constants.mapped, &gpuState, sizeof(gpuState));

		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = static_cast<float>(command.pipeline.viewport[0]);
		viewport.TopLeftY = static_cast<float>(state.targetHeight -
			(command.pipeline.viewport[1] + command.pipeline.viewport[3]));
		if (command.primitives->topology == legacygl::Topology::Lines)
		{
			// Select the GL diamond-exit side of exact pixel-boundary ties.
			viewport.TopLeftY += std::ldexp(1.0f, -D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT);
		}
		viewport.Width = static_cast<float>(command.pipeline.viewport[2]);
		viewport.Height = static_cast<float>(command.pipeline.viewport[3]);
		viewport.MaxDepth = 1.0f;
		const D3D12_RECT scissor = fullScissor();
		bindLegacyDynamicState(viewport, scissor);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeRenderTargetPassEmulationBegin(RenderTargetPassEmulation::Offscreen);
#endif
		bindOffscreenTargets(true, command.enables.colorLogicOp);
		// SetDescriptorHeaps can flush GPU state, so it is bound once per
		// command list rather than once per draw. Every path that binds a
		// different heap set invalidates the command-list state first.
		if (!state.commandListState.legacyDescriptorHeapsValid)
		{
			ID3D12DescriptorHeap *heaps[] = { state.srvHeap.Get(), state.samplerHeap.Get() };
			state.commandList->SetDescriptorHeaps(2, heaps);
			state.commandListState.legacyDescriptorHeapsValid = true;
		}
		bindLegacyRootSignature();
		bindLegacyPipeline(command);
		const D3D_PRIMITIVE_TOPOLOGY topology = primitiveTopology(command.primitives->topology);
		bindLegacyTopology(topology);
		state.commandList->IASetVertexBuffers(0, 1, &vertexView);
		state.commandList->SetGraphicsRootConstantBufferView(0, constants.gpuAddress);
		state.commandList->SetGraphicsRootDescriptorTable(1, texture.srv);
		bindLegacySamplerTable(texture.sampler);
		state.commandList->DrawInstanced(drawVertexCount, 1, 0, 0);
#if defined(A126_RENDER_DIAGNOSTICS_COMPILED)
		observeRenderTargetPassEmulationEnd();
#endif

		if (command.primitives->topology == legacygl::Topology::Lines &&
			command.pipeline.lineWidth != 1.0f && !state.lineWidthFallbackReported)
		{
			std::fprintf(stderr,
				"LegacyGL d3d12: line width %.3g is unavailable; using classified fallback width 1\n",
				command.pipeline.lineWidth);
			state.lineWidthFallbackReported = true;
		}
	}

	void resolvedTextureUpload(const legacygl::ResolvedTextureUpload &command) override
	{
		if (command.level < 0 || command.level >= D3D12_TEXTURE_LEVELS)
			return;
		D3D12Texture &texture = state.textures[command.texture];
		TextureLevel &level = texture.levels[command.level];
		if (!command.subImage)
		{
			level.width = command.width;
			level.height = command.height;
			level.defined = true;
			level.rgba.assign(static_cast<std::size_t>(command.width) *
				static_cast<std::size_t>(command.height) * 4, 0);
		}
		const legacygl::PixelTransferFormat *transfer =
			legacygl::unsignedBytePixelTransferFormat(command.sourceFormat);
		legacygl::PixelStorageFormat storage;
		if (command.sourceType != GL_UNSIGNED_BYTE || transfer == nullptr ||
			!legacygl::pixelStorageFormat(command.internalFormat, storage) ||
			storage.physical != legacygl::PhysicalPixelFormat::RGBA8)
		{
			throw std::runtime_error("D3D12 texture upload received an unsupported pixel format");
		}
		if (command.pixels != nullptr && command.width > 0 && command.height > 0)
		{
			const int components = transfer->components;
			const std::size_t sourceRow = alignedRowSize(static_cast<std::size_t>(command.width) *
				static_cast<std::size_t>(components), command.unpackAlignment);
			const unsigned char *source = static_cast<const unsigned char *>(command.pixels);
			for (int y = 0; y < command.height; y++)
			{
				for (int x = 0; x < command.width; x++)
				{
					unsigned char rgba[4];
					if (!legacygl::decodeUnsignedBytePixel(
						source + static_cast<std::size_t>(y) * sourceRow +
							static_cast<std::size_t>(x) * static_cast<std::size_t>(components),
						command.sourceFormat, rgba) ||
						!legacygl::applyIntendedPixelFormat(storage.intended, rgba))
					{
						throw std::runtime_error("D3D12 texture upload conversion failed");
					}
					const std::size_t destination = (static_cast<std::size_t>(command.y + y) *
						static_cast<std::size_t>(level.width) +
						static_cast<std::size_t>(command.x + x)) * 4;
					std::memcpy(level.rgba.data() + destination, rgba, 4);
				}
			}
		}
		texture.derivedDirty = true;
	}

	void resolvedReadback(const legacygl::ResolvedReadback &command) override
	{
		if (command.pixels == nullptr || command.width <= 0 || command.height <= 0 ||
			!ensureRenderTargets())
		{
			return;
		}
		readBackColor(command);
	}

private:
	LogicalNameAllocator textureNames;
	LogicalNameAllocator bufferNames;
	std::uint64_t nextListName = 1;
};

static D3D12Sink sinkInstance;

}

namespace renderbackend
{

static const Configuration &d3d12Configuration()
{
	static const Configuration value = {
		"d3d12",
		0,
		0,
		OpenGLProfile::None,
		0,
		0,
		OpenGLProfile::None,
		true,
		false
	};
	return value;
}

static void d3d12Initialize()
{
	d3d12backend::initialize();
}

static void d3d12Present()
{
	d3d12backend::present();
}

static void d3d12Shutdown()
{
	d3d12backend::shutdown();
}

static bool d3d12HasCapability(const char *capability)
{
	return capability != nullptr && std::strcmp(capability, "GL_NV_fog_distance") == 0;
}

static legacygl::Sink *d3d12Sink()
{
	return &d3d12backend::sinkInstance;
}

const Backend &d3d12Backend()
{
	static const Backend backend = {
		"d3d12",
		d3d12Configuration,
		d3d12Initialize,
		d3d12Present,
		d3d12Shutdown,
		d3d12HasCapability,
		d3d12Sink
	};
	return backend;
}

}
