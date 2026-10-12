#include "stdafx.h"

#ifdef MU_RMLUI

#include "UI/Modern/RmlRenderInterface.h"

#include "Core/Utilities/Log/MuLogger.h"

#include <SDL3/SDL_gpu.h>

#include <algorithm>
#include <cstring>

namespace UI::Modern
{
namespace
{
constexpr int kBytesPerPixel = 4;
constexpr std::uint32_t kMaxChannel = 255u;
constexpr std::uint8_t kOpaqueWhite[kBytesPerPixel] = {255, 255, 255, 255};

std::uint8_t Unpremultiply(std::uint8_t channel, std::uint8_t alpha)
{
    if (alpha == 0)
    {
        return 0;
    }
    const std::uint32_t straight = (static_cast<std::uint32_t>(channel) * kMaxChannel) / alpha;
    return static_cast<std::uint8_t>(std::min(straight, kMaxChannel));
}

// Vertex2D colours are packed ABGR (R in the lowest byte), as the GL layout expects.
std::uint32_t ToStraightAbgr(const ::Rml::ColourbPremultiplied& colour)
{
    const std::uint32_t r = Unpremultiply(colour.red, colour.alpha);
    const std::uint32_t g = Unpremultiply(colour.green, colour.alpha);
    const std::uint32_t b = Unpremultiply(colour.blue, colour.alpha);
    const std::uint32_t a = colour.alpha;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

std::vector<std::uint8_t> UnpremultiplyPixels(::Rml::Span<const ::Rml::byte> premultiplied)
{
    std::vector<std::uint8_t> straight(premultiplied.begin(), premultiplied.end());
    for (std::size_t i = 0; i + kBytesPerPixel <= straight.size(); i += kBytesPerPixel)
    {
        const std::uint8_t alpha = straight[i + 3];
        straight[i + 0] = Unpremultiply(straight[i + 0], alpha);
        straight[i + 1] = Unpremultiply(straight[i + 1], alpha);
        straight[i + 2] = Unpremultiply(straight[i + 2], alpha);
    }
    return straight;
}

SDL_GPUDevice* Device()
{
    return static_cast<SDL_GPUDevice*>(mu::GetRenderer().GetDevice());
}

bool UploadPixels(SDL_GPUDevice* device, SDL_GPUTexture* texture, const std::uint8_t* pixels, int width, int height)
{
    const Uint32 dataSize = static_cast<Uint32>(width) * static_cast<Uint32>(height) * kBytesPerPixel;

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = dataSize;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer)
    {
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped)
    {
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    std::memcpy(mapped, pixels, dataSize);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = commandBuffer ? SDL_BeginGPUCopyPass(commandBuffer) : nullptr;
    if (!copyPass)
    {
        if (commandBuffer)
        {
            SDL_SubmitGPUCommandBuffer(commandBuffer);
        }
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }

    SDL_GPUTextureTransferInfo src{};
    src.transfer_buffer = transfer;
    src.pixels_per_row = static_cast<Uint32>(width);
    src.rows_per_layer = static_cast<Uint32>(height);

    SDL_GPUTextureRegion dst{};
    dst.texture = texture;
    dst.w = static_cast<Uint32>(width);
    dst.h = static_cast<Uint32>(height);
    dst.d = 1;

    SDL_UploadToGPUTexture(copyPass, &src, &dst, false);
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return true;
}
} // namespace

RmlRenderInterface::RmlRenderInterface()
{
    m_whiteTexture = CreateGpuTexture(kOpaqueWhite, 1, 1);
}

RmlRenderInterface::~RmlRenderInterface()
{
    FlushReleasedTextures();
    DestroyGpuTexture(m_whiteTexture);
}

void RmlRenderInterface::BeginFrame(int windowWidth, int windowHeight)
{
    FlushReleasedTextures();
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    m_scissorEnabled = false;
}

void RmlRenderInterface::EndFrame()
{
    mu::GetRenderer().SetScissor(0, 0, m_windowWidth, m_windowHeight);
}

::Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(::Rml::Span<const ::Rml::Vertex> vertices,
                                                                  ::Rml::Span<const int> indices)
{
    auto* geometry = new CompiledGeometry();
    geometry->triangles.reserve(indices.size());
    for (const int index : indices)
    {
        const ::Rml::Vertex& source = vertices[static_cast<std::size_t>(index)];
        geometry->triangles.push_back(mu::Vertex2D{source.position.x, source.position.y, source.tex_coord.x,
                                                   source.tex_coord.y, ToStraightAbgr(source.colour)});
    }
    return reinterpret_cast<::Rml::CompiledGeometryHandle>(geometry);
}

void RmlRenderInterface::RenderGeometry(::Rml::CompiledGeometryHandle geometry, ::Rml::Vector2f translation,
                                        ::Rml::TextureHandle texture)
{
    const auto* compiled = reinterpret_cast<const CompiledGeometry*>(geometry);
    const auto* gpuTexture = texture ? reinterpret_cast<const GpuTexture*>(texture) : m_whiteTexture;
    if (!compiled || compiled->triangles.empty() || !gpuTexture)
    {
        return;
    }

    // The 2D triangle pipeline projects with a Y-up ortho matrix.
    const auto windowHeight = static_cast<float>(m_windowHeight);
    m_scratch.assign(compiled->triangles.begin(), compiled->triangles.end());
    for (mu::Vertex2D& vertex : m_scratch)
    {
        vertex.x += translation.x;
        vertex.y = windowHeight - (vertex.y + translation.y);
    }

    mu::GetRenderer().SubmitTextTriangles(m_scratch, gpuTexture->texture, gpuTexture->sampler);
}

void RmlRenderInterface::ReleaseGeometry(::Rml::CompiledGeometryHandle geometry)
{
    delete reinterpret_cast<CompiledGeometry*>(geometry);
}

::Rml::TextureHandle RmlRenderInterface::LoadTexture(::Rml::Vector2i& textureDimensions, const ::Rml::String& source)
{
    // Image files are not needed by the first HUD (styling is CSS only).
    // A PNG loader is added together with the first skinned element.
    (void)textureDimensions;
    mu::log::Get("rmlui")->warn("image textures are not supported yet: {}", source);
    return 0;
}

::Rml::TextureHandle RmlRenderInterface::GenerateTexture(::Rml::Span<const ::Rml::byte> source,
                                                         ::Rml::Vector2i sourceDimensions)
{
    const std::vector<std::uint8_t> straight = UnpremultiplyPixels(source);
    return reinterpret_cast<::Rml::TextureHandle>(
        CreateGpuTexture(straight.data(), sourceDimensions.x, sourceDimensions.y));
}

void RmlRenderInterface::ReleaseTexture(::Rml::TextureHandle texture)
{
    if (texture)
    {
        m_pendingRelease.push_back(reinterpret_cast<GpuTexture*>(texture));
    }
}

void RmlRenderInterface::EnableScissorRegion(bool enable)
{
    m_scissorEnabled = enable;
    ApplyScissor();
}

void RmlRenderInterface::SetScissorRegion(::Rml::Rectanglei region)
{
    m_scissorRegion = region;
    ApplyScissor();
}

void RmlRenderInterface::ApplyScissor()
{
    if (!m_scissorEnabled)
    {
        mu::GetRenderer().SetScissor(0, 0, m_windowWidth, m_windowHeight);
        return;
    }
    mu::GetRenderer().SetScissor(m_scissorRegion.Left(), m_scissorRegion.Top(), m_scissorRegion.Width(),
                                 m_scissorRegion.Height());
}

RmlRenderInterface::GpuTexture* RmlRenderInterface::CreateGpuTexture(const std::uint8_t* straightRgba, int width,
                                                                     int height)
{
    SDL_GPUDevice* device = Device();
    if (!device || !straightRgba || width <= 0 || height <= 0)
    {
        return nullptr;
    }

    SDL_GPUTextureCreateInfo textureInfo{};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.width = static_cast<Uint32>(width);
    textureInfo.height = static_cast<Uint32>(height);
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    auto gpuTexture = std::make_unique<GpuTexture>();
    gpuTexture->texture = SDL_CreateGPUTexture(device, &textureInfo);
    if (!gpuTexture->texture || !UploadPixels(device, gpuTexture->texture, straightRgba, width, height))
    {
        DestroyGpuTexture(gpuTexture.release());
        return nullptr;
    }

    SDL_GPUSamplerCreateInfo samplerInfo{};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    gpuTexture->sampler = SDL_CreateGPUSampler(device, &samplerInfo);
    if (!gpuTexture->sampler)
    {
        DestroyGpuTexture(gpuTexture.release());
        return nullptr;
    }
    return gpuTexture.release();
}

void RmlRenderInterface::DestroyGpuTexture(GpuTexture* texture)
{
    if (!texture)
    {
        return;
    }
    SDL_GPUDevice* device = Device();
    if (device)
    {
        if (texture->sampler)
        {
            SDL_ReleaseGPUSampler(device, texture->sampler);
        }
        if (texture->texture)
        {
            SDL_ReleaseGPUTexture(device, texture->texture);
        }
    }
    delete texture;
}

void RmlRenderInterface::FlushReleasedTextures()
{
    for (GpuTexture* texture : m_pendingRelease)
    {
        DestroyGpuTexture(texture);
    }
    m_pendingRelease.clear();
}
} // namespace UI::Modern

#endif // MU_RMLUI
