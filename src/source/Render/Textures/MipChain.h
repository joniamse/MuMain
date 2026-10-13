#pragma once

#include <cstdint>
#include <vector>

// CPU-built mip chains for RGBA8 textures ([Modern] EnhancedTextures). Built on the CPU so
// the GPU textures stay sampler-only: generating mips on the GPU needs colour-target
// textures, and Direct3D 12 runs out of render-target descriptors with thousands of them.
namespace Render::Textures::MipChain
{
struct Level
{
    int width;
    int height;
    std::uint32_t offset; // byte offset of the level inside the chain buffer
};

// Level 0 is a copy of `pixels`; each next level halves the size (at least 1) with a 2x2
// box filter. Colour is averaged weighted by alpha (no dark fringes around cut-outs) and
// alpha takes the maximum, so alpha-tested foliage keeps its coverage at a distance.
// Every level starts at a 512-byte aligned offset in the returned buffer.
std::vector<std::uint8_t> Build(const std::uint8_t* pixels, int width, int height, std::vector<Level>& levels);
} // namespace Render::Textures::MipChain
