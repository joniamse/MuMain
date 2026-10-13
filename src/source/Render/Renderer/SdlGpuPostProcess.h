#pragma once

#include "Render/Renderer/MuRenderer.h"

#include <SDL3/SDL_gpu.h>

namespace Render
{
// Screen-space post-processing for the SDL GPU renderer. The renderer draws the 3D world
// into SceneTarget(); Apply() then runs bloom (bright-pass + Gaussian blur at quarter
// resolution) and the composite pass (FXAA, sharpening, bloom, colour enhancement,
// vignette) into the destination, after which the UI is drawn on top.
class SdlGpuPostProcess
{
public:
    struct Shaders
    {
        SDL_GPUShader* fullscreenVertex = nullptr;
        SDL_GPUShader* bloomExtract = nullptr;
        SDL_GPUShader* blur = nullptr;
        SDL_GPUShader* composite = nullptr;
    };

    // `colorFormat` is the swapchain format; every target of the chain uses it.
    bool Create(SDL_GPUDevice* device, SDL_GPUTextureFormat colorFormat, const Shaders& shaders);
    void Destroy();
    [[nodiscard]] bool IsReady() const
    {
        return m_compositePipeline != nullptr;
    }

    // Colour target for the 3D world, (re)created to match the swapchain size.
    SDL_GPUTexture* SceneTarget(Uint32 width, Uint32 height);

    // Processes SceneTarget() into `destination` (same size). No render pass may be open.
    void Apply(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* destination,
               const mu::PostProcessSettings& settings);

private:
    struct Uniforms
    {
        float texel[4];
        float params[4];
        Uint32 flags[4];
    };

    SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUShader* fragmentShader) const;
    bool EnsureTargets(Uint32 width, Uint32 height);
    void ReleaseTargets();
    void RunPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUGraphicsPipeline* pipeline,
                 SDL_GPUTexture* destination, Uint32 width, Uint32 height,
                 const SDL_GPUTextureSamplerBinding* inputs, Uint32 inputCount, const Uniforms& uniforms) const;
    void RunBloom(SDL_GPUCommandBuffer* commandBuffer) const;

    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_format = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUShader* m_vertexShader = nullptr;
    SDL_GPUGraphicsPipeline* m_extractPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_blurPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_compositePipeline = nullptr;
    SDL_GPUSampler* m_sampler = nullptr;
    SDL_GPUTexture* m_scene = nullptr;
    SDL_GPUTexture* m_bloomA = nullptr;
    SDL_GPUTexture* m_bloomB = nullptr;
    Uint32 m_width = 0;
    Uint32 m_height = 0;
    Uint32 m_bloomWidth = 0;
    Uint32 m_bloomHeight = 0;
};
} // namespace Render
