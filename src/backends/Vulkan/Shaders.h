#pragma once

#include <cstddef>
#include <cstdint>

namespace vulkanbackend
{

const std::uint32_t *legacyVertexShaderCode(std::size_t &byteSize);
const std::uint32_t *legacyFragmentShaderCode(std::size_t &byteSize);
const std::uint32_t *clearVertexShaderCode(std::size_t &byteSize);
const std::uint32_t *clearFragmentShaderCode(std::size_t &byteSize);
const std::uint32_t *presentVertexShaderCode(std::size_t &byteSize);
const std::uint32_t *presentFragmentShaderCode(std::size_t &byteSize);

}
