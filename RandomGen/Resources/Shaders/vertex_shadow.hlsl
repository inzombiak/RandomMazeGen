#include "default_render_defs.hlsli"

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL0;
    float3 uv : TEXCOORD0;
    uint   instanceid : SV_InstanceID;
};

struct VertexOutput
{
    float4 color   : COLOR;
    float4 hpos    : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.hpos = mul(PerEntitySB[input.instanceid].M, float4(input.position, 1.0f));
    output.hpos = mul(SceneDataCB.sunVP, output.hpos);
    output.color = float4(input.color, 1.0f);
    
    return output;
}