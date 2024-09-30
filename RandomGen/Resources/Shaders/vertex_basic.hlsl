struct VertexInput
{
    float3 position : POSITION;
    float3 color    : COLOR;
    float3 normal   : NORMAL0;
    float3 uv       : TEXCOORD0;
    uint instanceid : SV_InstanceID;
};

struct Model
{
    matrix M;
};
StructuredBuffer<Model> ModelSB : register(t0);

struct PerEntityData
{
    uint type;
};
StructuredBuffer<PerEntityData> PerEntitySB : register(t1);

struct SceneData
{
    matrix camVP;
    matrix sunVP; 
};
ConstantBuffer<SceneData> SceneDataCB : register(b0);

struct VertexOutput
{   
    float3 normal   : NORMAL0;
    float4 color    : COLOR;
    float3 uv       : TEXCOORD0;
    float4 sunPos   : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    uint instanceid : SV_InstanceID;
    float4 hpos     : SV_Position;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.worldPos     = mul(ModelSB[input.instanceid].M, float4(input.position, 1.0f)).xyz;
    output.sunPos       = mul(SceneDataCB.sunVP, float4(output.worldPos, 1.f));
    output.hpos         = mul(SceneDataCB.camVP, float4(output.worldPos, 1.f));
    output.color        = float4(input.color, 1.0f);
    output.uv           = input.uv;
    output.instanceid   = input.instanceid;
    output.normal       = input.normal;
    return output;
}