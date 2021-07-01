
#include "PBR.hlsl"
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "VertexOutputs.hlsl"


struct GBufferFragOutput
{
    float4 diffuse : SV_Target0;
    float2 normal : SV_Target1;
    float4 specularRoughness : SV_Target2;
    float2 velocity : SV_Target3;
    float4 emissiveOcclusion : SV_Target4;
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

    const float2 velocity = (vertInput.curPosition.xy / vertInput.curPosition.w) - (vertInput.prevPosition.xy / vertInput.prevPosition.w);

    GBufferFragOutput output;
	output.diffuse = material.diffuse;
	output.normal = encodeOct(material.normal.xyz);
	output.specularRoughness = material.specularRoughness;
    output.velocity = velocity;
    output.emissiveOcclusion = material.emissiveOcclusion;

    return output;
}