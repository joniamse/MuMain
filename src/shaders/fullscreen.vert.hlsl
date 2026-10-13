// fullscreen.vert.hlsl - one triangle covering the screen, no vertex buffer.
// Vertices 0, 1, 2 map to UV (0,0), (2,0), (0,2); UV (0,0) is the top-left of the image.
struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

VSOutput main(uint vertexId : SV_VertexID)
{
    VSOutput output;
    const float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    output.uv = uv;
    output.pos = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 0.0, 1.0);
    return output;
}
