
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "VertexOutputs.hlsl"


struct GBufferFragOutput
{
    float4 albedo;
    float3 normal;
    float2 metalnessRoughness;
    float2 velocity;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];

[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


#include "PBR.hlsl"


GBufferFragOutput main(GBufferVertOutput vertInput)
{
    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal, 
                                                    model.materialAttributes, 
                                                    vertInput.materialID, 
                                                    viewDir, 
                                                    vertInput.uv);

    if(material.albedoOrDiffuse.w == 0.0f)
        discard;

    GBufferFragOutput output;
	output.albedo = material.albedoOrDiffuse;
	output.normal = (material.normal.xyz + 1.0f) * 0.5f;
	output.metalnessRoughness = float2(material.metalnessOrSpecular.x, material.roughness);
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}