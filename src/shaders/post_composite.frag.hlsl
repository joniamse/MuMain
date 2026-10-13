// post_composite.frag.hlsl - final image of the 3D world: FXAA, sharpening, bloom,
// colour enhancement and vignette, each switchable.
// texel.xy: one screen pixel in UV.
// params: x sharpening amount, y bloom strength, z vibrance, w vignette strength.
// flags:  x FXAA, y sharpening, z colour enhancement (vibrance, contrast, vignette), w bloom.
Texture2D sceneTex : register(t0, space2);
SamplerState sceneSampler : register(s0, space2);
Texture2D bloomTex : register(t1, space2);
SamplerState bloomSampler : register(s1, space2);
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

static const float3 kLumaWeights = float3(0.299, 0.587, 0.114);
static const float kFxaaReduceMin = 1.0 / 128.0;
static const float kFxaaReduceMul = 1.0 / 8.0;
static const float kFxaaSpanMax = 8.0;
static const float kContrast = 1.06;

float3 Scene(float2 uv)
{
    return sceneTex.Sample(sceneSampler, uv).rgb;
}

float Luma(float3 color)
{
    return dot(color, kLumaWeights);
}

// FXAA (Lottes' "FXAA 2" variant): blends along the local edge direction.
float3 Fxaa(float2 uv, float2 pixel)
{
    const float3 rgbNW = Scene(uv + float2(-1.0, -1.0) * pixel);
    const float3 rgbNE = Scene(uv + float2(1.0, -1.0) * pixel);
    const float3 rgbSW = Scene(uv + float2(-1.0, 1.0) * pixel);
    const float3 rgbSE = Scene(uv + float2(1.0, 1.0) * pixel);
    const float3 rgbM = Scene(uv);

    const float lumaNW = Luma(rgbNW);
    const float lumaNE = Luma(rgbNE);
    const float lumaSW = Luma(rgbSW);
    const float lumaSE = Luma(rgbSE);
    const float lumaM = Luma(rgbM);
    const float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    const float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    float2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    const float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * kFxaaReduceMul), kFxaaReduceMin);
    const float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, -kFxaaSpanMax, kFxaaSpanMax) * pixel;

    const float3 rgbA = 0.5 * (Scene(uv + dir * (1.0 / 3.0 - 0.5)) + Scene(uv + dir * (2.0 / 3.0 - 0.5)));
    const float3 rgbB = rgbA * 0.5 + 0.25 * (Scene(uv + dir * -0.5) + Scene(uv + dir * 0.5));
    const float lumaB = Luma(rgbB);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

// Unsharp mask on the 4 direct neighbours.
float3 Sharpen(float3 center, float2 uv, float2 pixel, float amount)
{
    const float3 neighbours = Scene(uv + float2(pixel.x, 0.0)) + Scene(uv - float2(pixel.x, 0.0)) +
                              Scene(uv + float2(0.0, pixel.y)) + Scene(uv - float2(0.0, pixel.y));
    return center + (center * 4.0 - neighbours) * (amount * 0.25);
}

// Vibrance boosts dull colours more than saturated ones; then a gentle contrast curve.
float3 Enhance(float3 color, float vibrance)
{
    const float maxChannel = max(color.r, max(color.g, color.b));
    const float minChannel = min(color.r, min(color.g, color.b));
    const float saturation = maxChannel - minChannel;
    const float luma = Luma(color);
    color = lerp(luma.xxx, color, 1.0 + vibrance * (1.0 - saturation));
    return (color - 0.5) * kContrast + 0.5;
}

float Vignette(float2 uv, float strength)
{
    const float distance = length(uv - 0.5) * 1.41421356;
    return 1.0 - strength * smoothstep(0.55, 1.0, distance);
}

float4 main(FSInput input) : SV_Target
{
    const float2 pixel = texel.xy;
    float3 color = flags.x != 0 ? Fxaa(input.uv, pixel) : Scene(input.uv);

    if (flags.y != 0)
    {
        color = Sharpen(color, input.uv, pixel, params.x);
    }
    if (flags.w != 0)
    {
        color += bloomTex.Sample(bloomSampler, input.uv).rgb * params.y;
    }
    if (flags.z != 0)
    {
        color = Enhance(color, params.z) * Vignette(input.uv, params.w);
    }
    return float4(saturate(color), 1.0);
}
