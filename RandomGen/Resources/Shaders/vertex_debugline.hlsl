#include "default_render_defs.hlsli"

// Physics debug lines. Positions arrive already in world space -- the debug
// drawer emits world-space endpoints -- so this only applies the camera.

struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
};

struct VertexOutput
{
    float4 color : COLOR;
    float4 hpos  : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.hpos  = mul(SceneDataCB.camVP, float4(input.position, 1.0f));
    output.color = float4(input.color, 1.0f);
    return output;
}
