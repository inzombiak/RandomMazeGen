struct PixelInput
{
    float4 color : COLOR;
    float3 uv : TEXCOORD0;
    uint   instanceid : SV_InstanceID;
};

struct PerEntityData
{
    uint type;
};
StructuredBuffer<PerEntityData> PerEntitySB : register(t1);

Texture2D wallTexture : register(t2);
Texture2D grassTexture : register(t3);
Texture2D dirtTexture : register(t4);

SamplerState TextureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

float4 main(PixelInput input) : SV_Target
{
    PerEntityData ped = PerEntitySB[input.instanceid];
    if (input.uv.z > 0.5)
    {
        if (ped.type != 1)
        {
            return grassTexture.Sample(TextureSampler, input.uv.xy) * input.color;
        }
        else
        {
            return wallTexture.Sample(TextureSampler, float2(input.uv.x, input.uv.y));
        }
    }
    else
    {
        return dirtTexture.Sample(TextureSampler, input.uv.xy);
    }
   
}