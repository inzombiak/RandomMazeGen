#include "default_render_defs.hlsli"

struct PixelInput
{
    float3 normal : NORMAL0;
    float4 color : COLOR;
    float3 uv : TEXCOORD0;
    float4 sunPos : TEXCOORD1;
    float3 worldPos : TEXCOORD2;
    uint   instanceid : SV_InstanceID;
};

Texture2D wallTexture : register(t1);
Texture2D grassTexture : register(t2);
Texture2D dirtTexture : register(t3);
Texture2D shadowTexture : register(t4);
SamplerState TextureSampler : register(s0);
/*~
    StaticSampler
    {
        Register: 0
    }
*/
SamplerComparisonState ShadowSampler : register(s1);
/*~
    StaticSampler
    {
        Filter: CompMinMagLinearMipPoint
        CompFunc: Less
        Register: 1
    }
*/

float ShadowCalculation(float3 surfaceNormal, float4 fragPosLightSpace, float3 lightDir)
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
    float bias = max(0.003 * (1.0 - dot(surfaceNormal, lightDir)), 0.002);

    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            shadow += shadowTexture.SampleCmpLevelZero(ShadowSampler, float2(projCoords.x, 1 - projCoords.y) + float2(x, y) * SceneDataCB.shadowTexelSize, z - bias).r;
        }
    }
    shadow /= 9.0;

    return 1 - shadow;
}

float4 main(PixelInput input) : SV_Target
{
    PerEntityData ped = PerEntitySB[input.instanceid];
    float4 color;
    if (input.uv.z > 0.5)
    {
        if (ped.data != 1)
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
    float3 lightDir = normalize(SceneDataCB.lightPos.xyz - input.worldPos.xyz);
    
    float3 normal = input.normal;
    float diff = max(dot(lightDir, normal), 0.0);
    float3 diffuse = diff * lightColor;
    // specular
    float3 viewDir = normalize(SceneDataCB.viewPos.xyz - input.worldPos.xyz);
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
    float shadow = ShadowCalculation(normal, input.sunPos, lightDir);
    
    float3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color.xyz;
   //return float4(z, projCoords.x, 1 - projCoords.y, projCoords.z);
    return float4(lighting, 1.0);
}

/*~
    RenderTargets 
    {
        RGBA8_UNORM
    }
*/