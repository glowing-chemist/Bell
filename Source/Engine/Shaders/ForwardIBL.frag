#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "PBR.hlsl"


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
TextureCube<float4> ConvolvedSkyboxSpecular;

[[vk::binding(3)]]
TextureCube<float4> ConvolvedSkyboxDiffuse;

[[vk::binding(4)]]
SamplerState linearSampler;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map) || defined(RayTraced_Shadows)
[[vk::binding(5)]]
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

    float3 irradiance = ConvolvedSkyboxDiffuse.Sample(linearSampler, material.normal.xyz).xyz;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map) || defined(RayTraced_Shadows)
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(material.normal.xyz, viewDir);
    const float3 dfg = DFG.Sample(linearSampler, float2(NoV, roughness * roughness));

#if MATERIAL_FLAGS & kMaterial_AlphaTested
    if(material.diffuse.w == 0.0f)
        discard;
#endif

    const float3 diffuse = calculateDiffuseDisney(material, irradiance, dfg);
    const float3 specular = calculateSpecular(  material,
                                                radiance, 
                                                dfg);

    Output output;

    output.colour = float4(specular + diffuse + material.emissiveOcclusion.xyz, 1.0) * material.emissiveOcclusion.w;
    output.velocity = (((vertInput.curPosition.xy / vertInput.curPosition.w) * 0.5f + 0.5f) - ((vertInput.prevPosition.xy / vertInput.prevPosition.w) * 0.5f + 0.5f));

    return output;
}