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
Texture2D<float2> DFG;

[[vk::binding(2)]]
Texture2D<float4> ltcMat;

[[vk::binding(3)]]
Texture2D<float2> ltcAmp;

[[vk::binding(4)]]
Texture2D<uint> activeFroxels;


[[vk::binding(5)]]
TextureCube<float4> skyBox;

[[vk::binding(6)]]
TextureCube<float4> ConvolvedSkybox;

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


Output main(GBufferVertOutput vertInput)
{
    const float4 baseAlbedo = materials[vertInput.materialID * 4].Sample(linearSampler, vertInput.uv);

    float3 normal = materials[(vertInput.materialID * 4) + 1].Sample(linearSampler, vertInput.uv).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = materials[(vertInput.materialID * 4) + 2].Sample(linearSampler, vertInput.uv).x;

    const float metalness = materials[(vertInput.materialID * 4) + 3].Sample(linearSampler, vertInput.uv);

    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

	const float2 xDerivities = ddx_fine(vertInput.uv);
	const float2 yDerivities = ddy_fine(vertInput.uv);

	{
    	float3x3 tbv = tangentSpaceMatrix(vertInput.normal, viewDir, float4(xDerivities, yDerivities));

    	normal = mul(normal, tbv);

    	normal = normalize(normal);
	}

	const float3 lightDir = reflect(-viewDir, normal);

	const float lodLevel = roughness * 10.0f;

    // Calculate contribution from enviroment.
    float3 radiance = ConvolvedSkybox.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = skyBox.Sample(linearSampler, normal).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(normal, viewDir);
    float3x3 minV;
    float LTCAmp;
    float2 f_ab;
    initializeLightState(minV, LTCAmp, f_ab, DFG, ltcMat, ltcAmp, linearSampler, NoV, roughness);

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const float3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

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
                lighting += pointLightContribution(light, vertInput.positionWS, viewDir, normal, metalness, roughness, baseAlbedo.xyz, f_ab);
                break;
            }

            case 1: // Spot.
            {
                lighting +=  pointLightContribution(light, vertInput.positionWS, viewDir, normal, metalness, roughness, baseAlbedo.xyz, f_ab);
                break;
            }

            case 2: // Area.
            {
                lighting +=  areaLightContribution(light, vertInput.positionWS, viewDir, normal, metalness, roughness, baseAlbedo.xyz, minV, LTCAmp);
                break;
            }

            // Should never hit here.
            default:
                continue;
        }
    }

    if(baseAlbedo.w == 0.0f)
        discard;

    Output output;

    output.colour = lighting;
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}