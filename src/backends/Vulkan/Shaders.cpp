#include "backends/Vulkan/Shaders.h"

namespace vulkanbackend
{

static const std::uint32_t LEGACY_VERTEX_SHADER[] =
#include "LegacyFFP.vert.inc"
;

static const std::uint32_t LEGACY_FRAGMENT_SHADER[] =
#include "LegacyFFP.frag.inc"
;

static const std::uint32_t CLEAR_VERTEX_SHADER[] =
#include "Clear.vert.inc"
;

static const std::uint32_t CLEAR_FRAGMENT_SHADER[] =
#include "Clear.frag.inc"
;

static const std::uint32_t PRESENT_VERTEX_SHADER[] =
#include "Present.vert.inc"
;

static const std::uint32_t PRESENT_FRAGMENT_SHADER[] =
#include "Present.frag.inc"
;

const std::uint32_t *legacyVertexShaderCode(std::size_t &byteSize)
{
	byteSize = sizeof(LEGACY_VERTEX_SHADER);
	return LEGACY_VERTEX_SHADER;
}

const std::uint32_t *legacyFragmentShaderCode(std::size_t &byteSize)
{
	byteSize = sizeof(LEGACY_FRAGMENT_SHADER);
	return LEGACY_FRAGMENT_SHADER;
}

const std::uint32_t *clearVertexShaderCode(std::size_t &byteSize)
{
	byteSize = sizeof(CLEAR_VERTEX_SHADER);
	return CLEAR_VERTEX_SHADER;
}

const std::uint32_t *clearFragmentShaderCode(std::size_t &byteSize)
{
	byteSize = sizeof(CLEAR_FRAGMENT_SHADER);
	return CLEAR_FRAGMENT_SHADER;
}

const std::uint32_t *presentVertexShaderCode(std::size_t &byteSize)
{
	byteSize = sizeof(PRESENT_VERTEX_SHADER);
	return PRESENT_VERTEX_SHADER;
}

const std::uint32_t *presentFragmentShaderCode(std::size_t &byteSize)
{
	byteSize = sizeof(PRESENT_FRAGMENT_SHADER);
	return PRESENT_FRAGMENT_SHADER;
}

}
