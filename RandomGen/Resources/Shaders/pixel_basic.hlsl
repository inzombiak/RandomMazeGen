struct PixelInput
{
    float3 normal : NORMAL0;
    float4 color : COLOR;
    float3 uv : TEXCOORD0;
    float4 sunPos : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    uint   instanceid : SV_InstanceID;
};

cbuffer LightingPos : register(b0)
{
    float4 lightPos;
    float4 viewPos;
    float2 shadowTexelSize;
};

struct PerEntityData
{
    uint type;
};
StructuredBuffer<PerEntityData> PerEntitySB : register(t1);

Texture2D wallTexture : register(t2);
Texture2D grassTexture : register(t3);
Texture2D dirtTexture : register(t4);
Texture2D shadowTexture : register(t5);
SamplerState TextureSampler : register(s0)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

SamplerComparisonState ShadowSampler : register(s1);

float ShadowCalculation(float4 fragPosLightSpace)
{
     // perform perspective divide
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    float z = projCoords.z;
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    //float closestDepth = shadowTexture.Sample(TextureSampler, float2(projCoords.x, 1 - projCoords.y)).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = 0.001;
    //float shadow = z - bias > closestDepth ? 1.0 : 0.0;

    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            shadow += 1 - shadowTexture.SampleCmpLevelZero(ShadowSampler, float2(projCoords.x, 1 - projCoords.y) + float2(x, y) * shadowTexelSize, z - bias).r;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

float4 main(PixelInput input) : SV_Target
{
    PerEntityData ped = PerEntitySB[input.instanceid];
    float4 color;
    if (input.uv.z > 0.5)
    {
        if (ped.type != 1)
        {
            color = grassTexture.Sample(TextureSampler, input.uv.xy) * input.color;
        }
        else
        {
            color = wallTexture.Sample(TextureSampler, float2(input.uv.x, input.uv.y));
        }
    }
    else
    {
        color = dirtTexture.Sample(TextureSampler, input.uv.xy);
    }
    
    
    float3 lightColor = float3(1.0, 1.0, 1.0);
    // ambient
    float3 ambient = 0.35 * lightColor;
    // diffuse
    float3 lightDir = normalize(lightPos.xyz - input.worldPos.xyz);
    
    float3 normal = input.normal;
    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * lightColor;
    // specular
    float3 viewDir = normalize(viewPos.xyz - input.worldPos.xyz);
    float spec = 0.0;
    float3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    float3 specular = spec * lightColor;
    // calculate shadow
   // perform perspective divide
    float3 projCoords = input.sunPos.xyz / input.sunPos.w;
    // transform to [0,1] range
    float z = projCoords.z;
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = shadowTexture.Sample(TextureSampler, float2(projCoords.x, 1 - projCoords.y)).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = 0.001;
    float shadow = ShadowCalculation(input.sunPos);
    
    float3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color.xyz;
    
   //return float4(z, projCoords.x, 1 - projCoords.y, projCoords.z);
   return float4(lighting, 1.0);
}