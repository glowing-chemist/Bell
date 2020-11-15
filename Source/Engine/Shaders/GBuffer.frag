
#include "PBR.hlsl"
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "VertexOutputs.hlsl"


struct GBufferFragOutput
{
    float4 diffuse;
    float2 normal;
    float4 specularRoughness;
    float2 velocity;
    float4 emissiveOcclusion;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];


#include "Materials.hlsl"


GBufferFragOutput main(GBufferVertOutput vertInput)
{
    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

    MaterialInfo material = calculateMaterialInfo(  vertInput.normal,
                                                    vertInput.colour,
                                                    vertInput.materialIndex, 
                                                    vertInput.tangent,
                                                    viewDir, 
                                                    vertInput.uv);

#if MATERIAL_FLAGS & kMaterial_AlphaTested
    if(material.diffuse.w == 0.0f)
        discard;
#endif

    const float2 velocity = (((vertInput.curPosition.xy / vertInput.curPosition.w) * 0.5f + 0.5f) - ((vertInput.prevPosition.xy / vertInput.prevPosition.w) * 0.5f + 0.5f));

    GBufferFragOutput output;
	output.diffuse = material.diffuse;
	output.normal = encodeOct(material.normal.xyz);
	output.specularRoughness = material.specularRoughness;
    output.velocity = (velocity * 0.5f) + 0.5f;
    output.emissiveOcclusion = material.emissiveOcclusion;

    return output;
}