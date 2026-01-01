struct PerEntityData
{
    matrix M;
    uint data;
};
StructuredBuffer<PerEntityData> PerEntitySB : register(t0);

struct SceneData
{
    matrix camVP;
    matrix sunVP;
    float4 lightPos;
    float4 viewPos;
    float2 shadowTexelSize;
};
ConstantBuffer<SceneData> SceneDataCB : register(b0);