#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "ClusteredLighting.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
RWTexture3D<float4> voxelGrid;

[[vk::binding(3)]]
SamplerState linearSampler;

[[vk::binding(4)]]
TextureCube<float4> ConvolvedSkyboxDiffuse;

#ifdef Light_froxelation
[[vk::binding(5)]]
StructuredBuffer<uint2> sparseFroxelList;

[[vk::binding(6)]]
StructuredBuffer<uint> indicies;
#endif

#ifdef Shadow_Map
#ifdef Light_froxelation
[[vk::binding(7)]]
#else
[[vk::binding(5)]]
#endif
Texture2D<float> shadowMap;
#endif

// materials
[[vk::binding(0, 1)]]
ConstantBuffer<MaterialAttributes> materialFlags;

[[vk::binding(1, 1)]]
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
	const float2 screenUV = vertInput.position.xy / float2(128.0f, 128.0f);

	const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal, 
                                                    materialFlags.materialAttributes, 
                                                    vertInput.materialID, 
                                                    viewDir, 
                                                    vertInput.uv);

    float3 irradiance = ConvolvedSkyboxDiffuse.Sample(linearSampler, material.normal.xyz).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, screenUV);
    irradiance *= occlusion;
#endif

	float3 diffuse = calculateDiffuse(material, irradiance);

#ifdef Light_froxelation
	// TODO separate out diffuse form specular lighting as we only want to add diffuse.
#endif

	// TODO calculate voxel position and atomically blend this value to it. 
}