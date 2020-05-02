#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "ClusteredLighting.hlsl"

struct Output
{
    float4 colour;
    float velocity;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float3> DFG;

[[vk::binding(2)]]
Texture2D<float4> ltcMat;

[[vk::binding(3)]]
Texture2D<float2> ltcAmp;

[[vk::binding(4)]]
Texture2D<uint> activeFroxels;

[[vk::binding(5)]]
TextureCube<float4> ConvolvedSkyboxSpecular;

[[vk::binding(6)]]
TextureCube<float4> ConvolvedSkyboxDiffuse;

[[vk::binding(7)]]
SamplerState linearSampler;

[[vk::binding(8)]]
SamplerState pointSampler;

[[vk::binding(9)]]
StructuredBuffer<uint2> sparseFroxelList;

[[vk::binding(10)]]
StructuredBuffer<uint> indicies;

#ifdef Shadow_Map
[[vk::binding(11)]]
Texture2D<float> shadowMap;
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
                                                    vertInput.materialFlags, 
                                                    vertInput.materialIndex, 
                                                    viewDir, 
                                                    vertInput.uv);

	const float3 lightDir = reflect(-viewDir, material.normal);

	const float lodLevel = material.specularRoughness.w * 10.0f;

    // Calculate contribution from enviroment.
    float3 radiance = ConvolvedSkyboxSpecular.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = ConvolvedSkyboxDiffuse.Sample(linearSampler, material.normal.xyz).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(material.normal.xyz, viewDir);
    float3x3 minV;
    float LTCAmp;
    float3 dfg;
    initializeLightState(minV, LTCAmp, dfg, DFG, ltcMat, ltcAmp, linearSampler, NoV, material.specularRoughness.w);

    const float3 diffuse = calculateDiffuseDisney(material, irradiance, dfg);
    const float3 specular = calculateSpecular(  material,
                                                radiance, 
                                                dfg);

    float4 lighting = float4(diffuse + specular, 1.0f);

    // Calculate contribution from lights.
    const uint froxelIndex = activeFroxels.Sample(pointSampler, vertInput.position.xy / camera.frameBufferSize);
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

    if(material.diffuse.w == 0.0f)
        discard;

    Output output;

    output.colour = lighting;
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}