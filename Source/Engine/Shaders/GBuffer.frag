
#include "PBR.hlsl"
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "VertexOutputs.hlsl"


struct GBufferFragOutput
{
    float4 diffuse;
    float3 normal;
    float4 specularRoughness;
    float2 velocity;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
ConstantBuffer<MaterialAttributes> materialFlags;

[[vk::binding(1, 1)]]
Texture2D materials[];


#include "Materials.hlsl"


GBufferFragOutput main(GBufferVertOutput vertInput)
{
    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal, 
                                                    materialFlags.materialAttributes, 
                                                    vertInput.materialID, 
                                                    viewDir, 
                                                    vertInput.uv);

    if(material.diffuse.w == 0.0f)
        discard;

    GBufferFragOutput output;
	output.diffuse = material.diffuse;
	output.normal = (material.normal.xyz + 1.0f) * 0.5f;
	output.specularRoughness = material.specularRoughness;
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}