struct PixelInput
{
    float4 color : COLOR;
};

Texture2D wallTexture : register(t1);

SamplerState TextureSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

float4 main(PixelInput input) : SV_Target
{
    return wallTexture.Sample(TextureSampler, 0.5, 0.5);
}