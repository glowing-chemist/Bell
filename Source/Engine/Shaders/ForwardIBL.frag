#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "PBR.hlsl"


struct Output
{
    float4 colour : SV_Target0;
    float2 velocity : SV_Target1;
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
#define USING_SHADOWS 1
[[vk::binding(5)]]
Texture2D<float> shadowMap;
#else
#define USING_SHADOWS 0
#endif

#if defined(Screen_Space_Reflection) || defined(RayTraced_Reflections)
#define USING_SSR
[[vk::binding(4 + USING_SHADOWS + 1)]]
Texture2D<float4> reflectionMap;
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

    const float2 velocity = (vertInput.curPosition.xy / vertInput.curPosition.w) - (vertInput.prevPosition.xy / vertInput.prevPosition.w);
    output.colour = float4(specular + diffuse + material.emissiveOcclusion.xyz, 1.0) * material.emissiveOcclusion.w;
    output.velocity = velocity;

    return output;
}