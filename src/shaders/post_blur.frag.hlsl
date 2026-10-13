// post_blur.frag.hlsl - separable 9-tap Gaussian blur.
// params.xy: step between taps in UV (one texel along the blur direction).
Texture2D sourceTex : register(t0, space2);
SamplerState sourceSampler : register(s0, space2);
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
    // Linear-sampling Gaussian: 5 fetches at offsets between texels give 9 effective taps.
    static const float offsets[3] = {0.0, 1.3846153846, 3.2307692308};
    static const float weights[3] = {0.2270270270, 0.3162162162, 0.0702702703};

    const float2 stepUv = params.xy;
    float3 color = sourceTex.Sample(sourceSampler, input.uv).rgb * weights[0];
    [unroll] for (int i = 1; i < 3; ++i)
    {
        color += sourceTex.Sample(sourceSampler, input.uv + stepUv * offsets[i]).rgb * weights[i];
        color += sourceTex.Sample(sourceSampler, input.uv - stepUv * offsets[i]).rgb * weights[i];
    }
    return float4(color, 1.0);
}
