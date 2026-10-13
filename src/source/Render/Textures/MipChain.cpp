#include "stdafx.h"

#include "Render/Textures/MipChain.h"

#include <algorithm>
#include <cstring>

namespace Render::Textures::MipChain
{
namespace
{
constexpr int kChannels = 4;
constexpr int kAlpha = 3;
constexpr int kSamples = 4;
// Direct3D 12 texture copies read each subresource from a 512-byte aligned offset.
constexpr std::uint32_t kLevelOffsetAlignment = 512;

std::uint32_t LevelBytes(int width, int height)
{
    return static_cast<std::uint32_t>(width) * static_cast<std::uint32_t>(height) * kChannels;
}

std::uint32_t AlignUp(std::uint32_t value, std::uint32_t alignment)
{
    return (value + alignment - 1) / alignment * alignment;
}

// One destination pixel from the 2x2 block at (x*2, y*2) of the source level.
void DownsamplePixel(const std::uint8_t* source, int sourceWidth, int sourceHeight, int x, int y,
                     std::uint8_t* target)
{
    const int x0 = std::min(x * 2, sourceWidth - 1);
    const int y0 = std::min(y * 2, sourceHeight - 1);
    const int x1 = std::min(x0 + 1, sourceWidth - 1);
    const int y1 = std::min(y0 + 1, sourceHeight - 1);
    const std::uint8_t* samples[kSamples] = {
        source + (y0 * sourceWidth + x0) * kChannels, source + (y0 * sourceWidth + x1) * kChannels,
        source + (y1 * sourceWidth + x0) * kChannels, source + (y1 * sourceWidth + x1) * kChannels};

    std::uint32_t alphaSum = 0;
    std::uint8_t alphaMax = 0;
    std::uint32_t weighted[3] = {};
    std::uint32_t plain[3] = {};
    for (const std::uint8_t* sample : samples)
    {
        const std::uint8_t alpha = sample[kAlpha];
        alphaSum += alpha;
        alphaMax = std::max(alphaMax, alpha);
        for (int channel = 0; channel < 3; ++channel)
        {
            weighted[channel] += static_cast<std::uint32_t>(sample[channel]) * alpha;
            plain[channel] += sample[channel];
        }
    }

    for (int channel = 0; channel < 3; ++channel)
    {
        const std::uint32_t value = alphaSum > 0 ? weighted[channel] / alphaSum : plain[channel] / kSamples;
        target[channel] = static_cast<std::uint8_t>(value);
    }
    target[kAlpha] = alphaMax;
}
} // namespace

std::vector<std::uint8_t> Build(const std::uint8_t* pixels, int width, int height, std::vector<Level>& levels)
{
    levels.clear();
    std::uint32_t total = 0;
    for (int w = width, h = height;; w = std::max(w / 2, 1), h = std::max(h / 2, 1))
    {
        total = AlignUp(total, kLevelOffsetAlignment);
        levels.push_back({w, h, total});
        total += LevelBytes(w, h);
        if (w == 1 && h == 1)
        {
            break;
        }
    }

    std::vector<std::uint8_t> chain(total);
    std::memcpy(chain.data(), pixels, LevelBytes(width, height));
    for (std::size_t i = 1; i < levels.size(); ++i)
    {
        const Level& source = levels[i - 1];
        const Level& target = levels[i];
        const std::uint8_t* sourcePixels = chain.data() + source.offset;
        std::uint8_t* targetPixels = chain.data() + target.offset;
        for (int y = 0; y < target.height; ++y)
        {
            for (int x = 0; x < target.width; ++x)
            {
                DownsamplePixel(sourcePixels, source.width, source.height, x, y,
                                targetPixels + (y * target.width + x) * kChannels);
            }
        }
    }
    return chain;
}
} // namespace Render::Textures::MipChain
