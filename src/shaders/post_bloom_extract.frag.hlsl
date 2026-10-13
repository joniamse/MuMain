// post_bloom_extract.frag.hlsl - bright parts of the scene at reduced resolution.
// params.x: luminance threshold (0..1); texel.xy: size of one source texel in UV.
Texture2D sceneTex : register(t0, space2);
SamplerState sceneSampler : register(s0, space2);
cbuffer PostUniforms : register(b0, space3)
{
    float4 texel;
    float4 params;
    uint4 flags;
};

struct FSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 main(FSInput input) : SV_Target
{
    // 4 bilinear taps average a 4x4 source block, so small sparks do not flicker.
    const float2 offset = texel.xy;
    float3 color = sceneTex.Sample(sceneSampler, input.uv + float2(-offset.x, -offset.y)).rgb;
    color += sceneTex.Sample(sceneSampler, input.uv + float2(offset.x, -offset.y)).rgb;
    color += sceneTex.Sample(sceneSampler, input.uv + float2(-offset.x, offset.y)).rgb;
    color += sceneTex.Sample(sceneSampler, input.uv + float2(offset.x, offset.y)).rgb;
    color *= 0.25;

    const float luma = dot(color, float3(0.299, 0.587, 0.114));
    const float threshold = params.x;
    const float weight = saturate((luma - threshold) / max(1.0 - threshold, 0.0001));
    return float4(color * weight, 1.0);
}
