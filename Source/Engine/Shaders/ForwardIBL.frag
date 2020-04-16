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
Texture2D<float2> DFG;

[[vk::binding(2)]]
TextureCube<float4> skyBox;

[[vk::binding(3)]]
TextureCube<float4> ConvolvedSkybox;

[[vk::binding(4)]]
SamplerState linearSampler;

#ifdef Shadow_Map
[[vk::binding(5)]]
Texture2D<float> shadowMap;
#endif

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
[[vk::binding(0, 1)]]
Texture2D materials[];

[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;



#include "Materials.hlsl"


Output main(GBufferVertOutput vertInput)
{
    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal, 
                                                    model.materialAttributes, 
                                                    vertInput.materialID, 
                                                    viewDir, 
                                                    vertInput.uv);

	const float3 lightDir = reflect(-viewDir, material.normal.xyz);

	const float lodLevel = material.roughness * 10.0f;

	float3 radiance = ConvolvedSkybox.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = skyBox.Sample(linearSampler, material.normal.xyz).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(material.normal.xyz, viewDir);
    const float2 f_ab = DFG.Sample(linearSampler, float2(NoV, material.roughness));

    if(material.albedoOrDiffuse.w == 0.0f)
        discard;

    const float3 diffuse = calculateDiffuse(material, model.materialAttributes, irradiance);
    const float3 specular = calculateSpecular(  material,
                                                model.materialAttributes, 
                                                viewDir,
                                                radiance, 
                                                f_ab);

    Output output;

    output.colour = float4(specular + diffuse, 1.0);
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}