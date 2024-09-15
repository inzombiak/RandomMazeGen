struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
    float3 uv       : TEXCOORD0;
    uint instanceid : SV_InstanceID;
};

struct Model
{
    matrix M;
};
StructuredBuffer<Model> ModelSB : register(t0);


struct ViewProjection
{
    matrix VP;
};
ConstantBuffer<ViewProjection> ViewProjectionCB : register(b0);


struct VertexOutput
{
    float4 color : COLOR;
    float3 uv    : TEXCOORD0;
    float4 hpos  : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.hpos = mul(ModelSB[input.instanceid].M, float4(input.position, 1.0f));
    output.hpos = mul(ViewProjectionCB.VP, output.hpos);;
    output.color = float4(input.color, 1.0f);
    output.uv = input.uv;
    
    return output;
}