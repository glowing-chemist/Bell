#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "ClusteredLighting.hlsl"

struct Output
{
    float4 colour : SV_Target0;
    float2 velocity : SV_Target1;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(4)]]
Texture2D<float3> DFG;

[[vk::binding(5)]]
Texture2D<float4> ltcMat;

[[vk::binding(6)]]
Texture2D<float2> ltcAmp;

[[vk::binding(7)]]
Texture2D<uint> activeFroxels;

[[vk::binding(8)]]
TextureCube<float4> ConvolvedSkyboxSpecular;

[[vk::binding(9)]]
TextureCube<float4> ConvolvedSkyboxDiffuse;

[[vk::binding(10)]]
SamplerState linearSampler;

[[vk::binding(11)]]
SamplerState pointSampler;

[[vk::binding(12)]]
StructuredBuffer<uint2> sparseFroxelList;

[[vk::binding(13)]]
StructuredBuffer<uint> indicies;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map) || defined(RayTraced_Shadows)
#define USING_SHADOWS 1
[[vk::binding(14)]]
Texture2D<float> shadowMap;
#else
#define USING_SHADOWS 0
#endif

#if defined(Screen_Space_Reflection) || defined(RayTraced_Reflections)
#define USING_SSR
[[vk::binding(13 + USING_SHADOWS + 1)]]
Texture2D<float4> reflectionMap;
#endif

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
[[vk::binding(0, 1)]]
Texture2D materials[];

[[vk::binding(0, 2)]]
StructuredBuffer<uint4> lightCount;

[[vk::binding(1, 2)]]
StructuredBuffer<Light> sceneLights;


#include "Materials.hlsl"

Output main(GBufferVertOutput vertInput)
{
    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal,
                                                    vertInput.colour,
                                                    vertInput.materialIndex, 
                                                    vertInput.tangent,
                                                    viewDir,
                                                    vertInput.uv);

	const float3 lightDir = reflect(-viewDir, material.normal);

	float roughness = material.specularRoughness.w;
    const float lodLevel = roughness * 10.0f;

    // Calculate contribution from enviroment.
#if defined(USING_SSR)
    float3 radiance = reflectionMap.Sample(linearSampler, uv).xyz;
#else
    float3 radiance = ConvolvedSkyboxSpecular.SampleLevel(linearSampler, lightDir, lodLevel).xyz;
#endif

    float3 irradiance = ConvolvedSkyboxDiffuse.Sample(linearSampler, material.normal.xyz).xyz;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map) || defined(RayTraced_Shadows)
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(material.normal.xyz, viewDir);
    float3x3 minV;
    float LTCAmp;
    float3 dfg;
    initializeLightState(minV, LTCAmp, dfg, DFG, ltcMat, ltcAmp, linearSampler, NoV, roughness * roughness);

    const float3 diffuse = calculateDiffuseDisney(material, irradiance, dfg);
    const float3 specular = calculateSpecular(  material,
                                                radiance, 
                                                dfg);

    float4 lighting = float4(diffuse + specular + material.emissiveOcclusion.xyz, 1.0f) * material.emissiveOcclusion.w;

    // Calculate contribution from lights.
    const uint froxelIndex = activeFroxels.Load(uint3(vertInput.position.xy, 0));
    const uint2 lightListIndicies = sparseFroxelList[froxelIndex];

    for(uint i = 0; i < lightListIndicies.y; ++i)
    {
        uint indirectLightIndex = indicies[lightListIndicies.x + i];
        const Light light = sceneLights[indirectLightIndex];

        switch(uint(light.type))
        {
            case 0:
            {
                lighting += pointLightContribution( light, 
                                                    vertInput.positionWS, 
                                                    viewDir, 
                                                    material,
                                                    dfg);
                break;
            }

            case 1: // Spot.
            {
                lighting +=  spotLightContribution( light, 
                                                    vertInput.positionWS, 
                                                    viewDir, 
                                                    material,
                                                    dfg);
                break;
            }

            case 2: // Area.
            {
                lighting +=  areaLightContribution( light, 
                                                    vertInput.positionWS, 
                                                    viewDir, 
                                                    material,
                                                    minV, 
                                                    LTCAmp);
                break;
            }

            // Should never hit here.
            default:
                continue;
        }
    }

#if SHADE_FLAGS & kMaterial_AlphaTested
    if(material.diffuse.w == 0.0f)
        discard;
#endif

    Output output;

    output.colour = lighting;
    const float2 velocity = (vertInput.curPosition.xy / vertInput.curPosition.w) - (vertInput.prevPosition.xy / vertInput.prevPosition.w);
    output.velocity = velocity;

    return output;
}