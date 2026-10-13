#pragma once

#ifdef MU_RMLUI

#include "Render/Renderer/MuRenderer.h"

#include "UI/Modern/RmlIncludes.h"

#include <vector>

struct SDL_GPUTexture;
struct SDL_GPUSampler;

namespace UI::Modern
{
// Image sources of the form "legacy:<bitmap index>" show a texture the game already loaded
// (e.g. skill icon sheets), cropped in RML with the img "rect" attribute.
inline constexpr char kLegacyTextureScheme[] = "legacy:";

// Draws RmlUi geometry through the SDL GPU renderer as 2D screen-space triangles.
// RmlUi works with premultiplied alpha and a top-left origin; the renderer's 2D
// triangle path blends straight alpha in a bottom-left (Y-up) space, so colours
// and texels are un-premultiplied once and positions are flipped per draw.
class RmlRenderInterface final : public ::Rml::RenderInterface
{
public:
    RmlRenderInterface();
    ~RmlRenderInterface() override;

    RmlRenderInterface(const RmlRenderInterface&) = delete;
    RmlRenderInterface& operator=(const RmlRenderInterface&) = delete;

    // Window size in OS-window pixels; must be set before Context::Render each frame.
    void BeginFrame(int windowWidth, int windowHeight);
    // Restores the full-window scissor so later 2D draws are not clipped.
    void EndFrame();

    ::Rml::CompiledGeometryHandle CompileGeometry(::Rml::Span<const ::Rml::Vertex> vertices,
                                                  ::Rml::Span<const int> indices) override;
    void RenderGeometry(::Rml::CompiledGeometryHandle geometry, ::Rml::Vector2f translation,
                        ::Rml::TextureHandle texture) override;
    void ReleaseGeometry(::Rml::CompiledGeometryHandle geometry) override;

    ::Rml::TextureHandle LoadTexture(::Rml::Vector2i& textureDimensions, const ::Rml::String& source) override;
    ::Rml::TextureHandle GenerateTexture(::Rml::Span<const ::Rml::byte> source,
                                         ::Rml::Vector2i sourceDimensions) override;
    void ReleaseTexture(::Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(::Rml::Rectanglei region) override;

private:
    struct GpuTexture
    {
        SDL_GPUTexture* texture = nullptr;
        SDL_GPUSampler* sampler = nullptr;
        // False for textures borrowed from the game (see kLegacyTextureScheme): their GPU
        // texture belongs to the game's bitmap cache and the sampler is shared.
        bool owned = true;
    };

    struct CompiledGeometry
    {
        // Indexed RmlUi geometry expanded to a plain triangle list, top-left origin.
        std::vector<mu::Vertex2D> triangles;
    };

    GpuTexture* CreateGpuTexture(const std::uint8_t* straightRgba, int width, int height);
    GpuTexture* BorrowLegacyTexture(const ::Rml::String& source, ::Rml::Vector2i& textureDimensions);
    GpuTexture* LoadPngTexture(const ::Rml::String& source, ::Rml::Vector2i& textureDimensions);
    void DestroyGpuTexture(GpuTexture* texture);
    void FlushReleasedTextures();
    void ApplyScissor();

    GpuTexture* m_whiteTexture = nullptr;
    // Linear filtering for borrowed game textures, which the game samples with GL_NEAREST.
    SDL_GPUSampler* m_borrowedSampler = nullptr;
    // Textures released during a frame may still be referenced by that frame's
    // queued draw commands; they are destroyed at the start of the next frame.
    std::vector<GpuTexture*> m_pendingRelease;
    std::vector<mu::Vertex2D> m_scratch;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    bool m_scissorEnabled = false;
    ::Rml::Rectanglei m_scissorRegion{};
};
} // namespace UI::Modern

#endif // MU_RMLUI
