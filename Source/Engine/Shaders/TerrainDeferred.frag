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

GBufferFragOutput main()
{
	GBufferFragOutput output;
	output.diffuse = float4(0.65f, 0.65f, 0.65f, 1.0f);
	output.normal = encodeOct(float3(0.0f, 1.0f, 0.0f));
	output.specularRoughness = float4(0.0f, 0.0f, 0.0f, 1.0f);
	output.velocity = float2(0.5f, 0.5f);
	output.emissiveOcclusion = float4(0.0f, 0.0f, 0.0f, 1.0f);

	return output;
}