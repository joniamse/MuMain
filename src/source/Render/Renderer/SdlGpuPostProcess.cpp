#include "stdafx.h"

#include "Render/Renderer/SdlGpuPostProcess.h"

#include "Core/Utilities/Log/MuLogger.h"

#include <algorithm>

namespace Render
{
namespace
{
constexpr Uint32 kBloomDownscale = 4;
constexpr int kBloomBlurIterations = 2;
constexpr float kBloomThreshold = 0.72f;
constexpr float kSharpenAmount = 0.35f;
constexpr float kBloomStrength = 0.55f;
constexpr float kVibrance = 0.25f;
constexpr float kVignette = 0.22f;
constexpr Uint32 kFullscreenTriangleVertices = 3;

SDL_GPUTexture* CreateTarget(SDL_GPUDevice* device, SDL_GPUTextureFormat format, Uint32 width, Uint32 height)
{
    SDL_GPUTextureCreateInfo info{};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = format;
    info.width = width;
    info.height = height;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
    return SDL_CreateGPUTexture(device, &info);
}

void ReleaseTexture(SDL_GPUDevice* device, SDL_GPUTexture*& texture)
{
    if (texture)
    {
        SDL_ReleaseGPUTexture(device, texture);
        texture = nullptr;
    }
}

void ReleasePipeline(SDL_GPUDevice* device, SDL_GPUGraphicsPipeline*& pipeline)
{
    if (pipeline)
    {
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
        pipeline = nullptr;
    }
}
} // namespace

bool SdlGpuPostProcess::Create(SDL_GPUDevice* device, SDL_GPUTextureFormat colorFormat, const Shaders& shaders)
{
    Destroy();
    if (!device || !shaders.fullscreenVertex || !shaders.bloomExtract || !shaders.blur || !shaders.composite)
    {
        mu::log::Get("render")->warn("SDL_gpu -- post-processing unavailable: shaders missing");
        return false;
    }

    m_device = device;
    m_format = colorFormat;
    m_vertexShader = shaders.fullscreenVertex;
    m_extractPipeline = CreatePipeline(shaders.bloomExtract);
    m_blurPipeline = CreatePipeline(shaders.blur);
    m_compositePipeline = CreatePipeline(shaders.composite);

    SDL_GPUSamplerCreateInfo samplerInfo{};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    m_sampler = SDL_CreateGPUSampler(device, &samplerInfo);

    if (!m_extractPipeline || !m_blurPipeline || !m_compositePipeline || !m_sampler)
    {
        mu::log::Get("render")->warn("SDL_gpu -- post-processing unavailable: {}", SDL_GetError());
        Destroy();
        return false;
    }
    mu::log::Get("render")->info("SDL_gpu -- post-processing ready");
    return true;
}

void SdlGpuPostProcess::Destroy()
{
    if (!m_device)
    {
        return;
    }
    ReleaseTargets();
    ReleasePipeline(m_device, m_extractPipeline);
    ReleasePipeline(m_device, m_blurPipeline);
    ReleasePipeline(m_device, m_compositePipeline);
    if (m_sampler)
    {
        SDL_ReleaseGPUSampler(m_device, m_sampler);
        m_sampler = nullptr;
    }
    m_device = nullptr;
}

SDL_GPUGraphicsPipeline* SdlGpuPostProcess::CreatePipeline(SDL_GPUShader* fragmentShader) const
{
    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = m_format;

    SDL_GPUGraphicsPipelineCreateInfo info{};
    info.vertex_shader = m_vertexShader;
    info.fragment_shader = fragmentShader;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    info.target_info.color_target_descriptions = &colorTarget;
    info.target_info.num_color_targets = 1;
    info.target_info.has_depth_stencil_target = false;
    return SDL_CreateGPUGraphicsPipeline(m_device, &info);
}

SDL_GPUTexture* SdlGpuPostProcess::SceneTarget(Uint32 width, Uint32 height)
{
    if (!IsReady() || width == 0 || height == 0)
    {
        return nullptr;
    }
    return EnsureTargets(width, height) ? m_scene : nullptr;
}

bool SdlGpuPostProcess::EnsureTargets(Uint32 width, Uint32 height)
{
    if (m_scene && width == m_width && height == m_height)
    {
        return true;
    }

    ReleaseTargets();
    m_width = width;
    m_height = height;
    m_bloomWidth = std::max(width / kBloomDownscale, 1u);
    m_bloomHeight = std::max(height / kBloomDownscale, 1u);
    m_scene = CreateTarget(m_device, m_format, width, height);
    m_bloomA = CreateTarget(m_device, m_format, m_bloomWidth, m_bloomHeight);
    m_bloomB = CreateTarget(m_device, m_format, m_bloomWidth, m_bloomHeight);
    if (!m_scene || !m_bloomA || !m_bloomB)
    {
        mu::log::Get("render")->warn("SDL_gpu -- post-processing targets failed: {}", SDL_GetError());
        ReleaseTargets();
        return false;
    }
    return true;
}

void SdlGpuPostProcess::ReleaseTargets()
{
    ReleaseTexture(m_device, m_scene);
    ReleaseTexture(m_device, m_bloomA);
    ReleaseTexture(m_device, m_bloomB);
    m_width = m_height = 0;
}

void SdlGpuPostProcess::RunPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUGraphicsPipeline* pipeline,
                                SDL_GPUTexture* destination, Uint32 width, Uint32 height,
                                const SDL_GPUTextureSamplerBinding* inputs, Uint32 inputCount,
                                const Uniforms& uniforms) const
{
    SDL_GPUColorTargetInfo target{};
    target.texture = destination;
    target.load_op = SDL_GPU_LOADOP_DONT_CARE;
    target.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer, &target, 1, nullptr);
    if (!pass)
    {
        return;
    }
    const SDL_GPUViewport viewport{0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f};
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_SetGPUViewport(pass, &viewport);
    SDL_BindGPUFragmentSamplers(pass, 0, inputs, inputCount);
    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &uniforms, sizeof(uniforms));
    SDL_DrawGPUPrimitives(pass, kFullscreenTriangleVertices, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

void SdlGpuPostProcess::RunBloom(SDL_GPUCommandBuffer* commandBuffer) const
{
    Uniforms uniforms{};
    uniforms.texel[0] = 1.0f / static_cast<float>(m_width);
    uniforms.texel[1] = 1.0f / static_cast<float>(m_height);
    uniforms.params[0] = kBloomThreshold;
    const SDL_GPUTextureSamplerBinding scene{m_scene, m_sampler};
    RunPass(commandBuffer, m_extractPipeline, m_bloomA, m_bloomWidth, m_bloomHeight, &scene, 1, uniforms);

    const float stepX = 1.0f / static_cast<float>(m_bloomWidth);
    const float stepY = 1.0f / static_cast<float>(m_bloomHeight);
    const SDL_GPUTextureSamplerBinding fromA{m_bloomA, m_sampler};
    const SDL_GPUTextureSamplerBinding fromB{m_bloomB, m_sampler};
    for (int iteration = 0; iteration < kBloomBlurIterations; ++iteration)
    {
        Uniforms horizontal{};
        horizontal.params[0] = stepX;
        RunPass(commandBuffer, m_blurPipeline, m_bloomB, m_bloomWidth, m_bloomHeight, &fromA, 1, horizontal);
        Uniforms vertical{};
        vertical.params[1] = stepY;
        RunPass(commandBuffer, m_blurPipeline, m_bloomA, m_bloomWidth, m_bloomHeight, &fromB, 1, vertical);
    }
}

void SdlGpuPostProcess::Apply(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* destination,
                              const mu::PostProcessSettings& settings)
{
    if (!IsReady() || !m_scene || !destination)
    {
        return;
    }
    if (settings.bloom)
    {
        RunBloom(commandBuffer);
    }

    Uniforms uniforms{};
    uniforms.texel[0] = 1.0f / static_cast<float>(m_width);
    uniforms.texel[1] = 1.0f / static_cast<float>(m_height);
    uniforms.params[0] = kSharpenAmount;
    uniforms.params[1] = kBloomStrength;
    uniforms.params[2] = kVibrance;
    uniforms.params[3] = kVignette;
    uniforms.flags[0] = settings.antiAliasing ? 1u : 0u;
    uniforms.flags[1] = settings.sharpening ? 1u : 0u;
    uniforms.flags[2] = settings.colorEnhance ? 1u : 0u;
    uniforms.flags[3] = settings.bloom ? 1u : 0u;

    const SDL_GPUTextureSamplerBinding inputs[2] = {{m_scene, m_sampler}, {m_bloomA, m_sampler}};
    RunPass(commandBuffer, m_compositePipeline, destination, m_width, m_height, inputs, 2, uniforms);
}
} // namespace Render
