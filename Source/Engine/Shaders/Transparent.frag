#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "PBR.hlsl"


struct Output
{
    float4 colour;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float3> DFG;

[[vk::binding(2)]]
TextureCube<float4> ConvolvedSkyboxSpecular;

[[vk::binding(3)]]
SamplerState linearSampler;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map)
[[vk::binding(4)]]
Texture2D<float> shadowMap;
#endif

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
[[vk::binding(0, 1)]]
Texture2D materials[];


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

	const float3 lightDir = reflect(-viewDir, material.normal.xyz);

    const float roughness = material.specularRoughness.w;
	const float lodLevel = roughness * 10.0f;

	float3 radiance = ConvolvedSkyboxSpecular.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map)
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
#endif

	const float NoV = dot(material.normal.xyz, viewDir);
    const float3 dfg = DFG.Sample(linearSampler, float2(NoV, roughness * roughness));

    const float3 specular = calculateSpecular(  material,
                                                radiance, 
                                                dfg);

    Output output;

    output.colour = float4(specular + material.emissiveOcclusion.xyz, dot(specular, float3(1.0f, 1.0f, 1.0f)) / 3.0f) * material.emissiveOcclusion.w;

    return output;
}