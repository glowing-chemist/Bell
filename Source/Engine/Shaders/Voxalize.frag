#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "ClusteredLighting.hlsl"
#include "NormalMapping.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
ConstantBuffer<VoxelDimmensions> voxelDimmensions;

[[vk::binding(2)]]
RWTexture3D<float4> voxelGrid;

[[vk::binding(3)]]
Texture2D<float3> DFG;

[[vk::binding(4)]]
Texture2D<float4> ltcMat;

[[vk::binding(5)]]
Texture2D<float2> ltcAmp;

[[vk::binding(6)]]
SamplerState linearSampler;

[[vk::binding(7)]]
TextureCube<float4> ConvolvedSkyboxDiffuse;

#ifdef Light_froxelation
[[vk::binding(8)]]
StructuredBuffer<uint2> sparseFroxelList;

[[vk::binding(9)]]
StructuredBuffer<uint> indicies;

[[vk::binding(10)]]
Texture2D<uint> activeFroxels;
#endif

#ifdef Shadow_Map
#ifdef Light_froxelation
[[vk::binding(11)]]
#else
[[vk::binding(8)]]
#endif
Texture2D<float> shadowMap;
#endif

// materials
[[vk::binding(0, 1)]]
Texture2D materials[];


#ifdef Light_froxelation
[[vk::binding(0, 2)]]
StructuredBuffer<uint4> lightCount;

[[vk::binding(1, 2)]]
StructuredBuffer<Light> sceneLights;
#endif

#include "Materials.hlsl"

// Don't export anything this shader just writes to a uav.
void main(VoxalizeGeometryOutput vertInput)
{
    uint width, height, depth;
    voxelGrid.GetDimensions(width, height, depth);

	const float2 screenUV = vertInput.position.xy / float2(width, height);

	const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal, 
                                                    vertInput.materialFlags, 
                                                    vertInput.materialIndex, 
                                                    viewDir, 
                                                    vertInput.uv);

    float3 irradiance = ConvolvedSkyboxDiffuse.Sample(linearSampler, material.normal.xyz).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, screenUV);
    irradiance *= occlusion;
#endif

	float3 diffuse = calculateDiffuse(material, irradiance);

#ifdef Light_froxelation
    
    const float NoV = dot(material.normal.xyz, viewDir);
    float3x3 minV;
    float LTCAmp;
    float3 dfg;
    initializeLightState(minV, LTCAmp, dfg, DFG, ltcMat, ltcAmp, linearSampler, NoV, material.specularRoughness.w);

    // Calculate contribution from lights.
    const uint froxelIndex = activeFroxels.Sample(linearSampler, screenUV);
    const uint2 lightListIndicies = sparseFroxelList[froxelIndex];

    for(uint i = 0; i < lightListIndicies.y; ++i)
    {
        uint indirectLightIndex = indicies[lightListIndicies.x + i];
        const Light light = sceneLights[indirectLightIndex];

        switch(uint(light.type))
        {
            case 0: // point
            {
                diffuse += pointLightContributionDiffuse( light, 
                                                    float4(vertInput.positionWS, 1.0f), 
                                                    material,
                                                    dfg);
                break;
            }

            case 1: // Spot.
            {
                diffuse +=  spotLightContributionDiffuse( light, 
                                                    float4(vertInput.positionWS, 1.0f), 
                                                    material,
                                                    dfg);
                break;
            }

            case 2: // Area.
            {
                diffuse +=  areaLightContributionDiffuse( light, 
                                                    float4(vertInput.positionWS, 1.0f), 
                                                    viewDir, 
                                                    material,
                                                    minV);
                break;
            }

            // Should never hit here.
            default:
                continue;
        }
    }

    if(material.diffuse.w == 0.0f)
        discard;

#endif

    const float3 viewSpacePosition = mul(camera.view, float4(vertInput.positionWS, 1.0f)).xyz;
    const uint voxelDepth = uint(floor(lerp(0.0f, float(depth), -viewSpacePosition.z / camera.farPlane)));
    const float aspect = camera.frameBufferSize.x / camera.frameBufferSize.y;

    const float worldSpaceHeight = 2.0f * viewSpacePosition.z * tan(radians(camera.fov) * 0.5f);
    const float worldSpaceWidth = worldSpaceHeight * aspect;
    const uint2 viewSpaceXY = uint2((float2(viewSpacePosition.xy / float2(worldSpaceWidth, -worldSpaceHeight)) + float2(0.5f)) * voxelDimmensions.voxelVolumeSize);

    voxelGrid[uint3(viewSpaceXY, voxelDepth)] = float4(diffuse, 1.0f);
}