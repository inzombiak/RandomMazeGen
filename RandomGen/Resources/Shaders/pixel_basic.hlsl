struct PixelInput
{
    float4 color : COLOR;
    float3 uv    : TEXCOORD0;
};

Texture2D wallTexture : register(t1);
Texture2D grassTexture : register(t2);
Texture2D dirtTexture : register(t3);

SamplerState TextureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

float4 main(PixelInput input) : SV_Target
{
    if (input.uv.z > 0.5)
    {
        return grassTexture.Sample(TextureSampler, input.uv.xy) * input.color;    
    }
    else
    {
        return dirtTexture.Sample(TextureSampler, input.uv.xy);
    }
}