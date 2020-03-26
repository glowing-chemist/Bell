#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
Texture2D<float4> globalLighting;

[[vk::binding(1)]]
Texture2D<float4> analyticalLighting;

[[vk::binding(2)]]
Texture2D<float> ssao;

[[vk::binding(3)]]
SamplerState defaultSampler;


float4 main(PositionAndUVVertOutput vertOutput)
{
	const float4 globalLight = globalLighting.Sample(defaultSampler, vertOutput.uv);
	const float4 analyticalLight = analyticalLighting.Sample(defaultSampler, vertOutput.uv);
	const float4 lighting = globalLight + analyticalLight;
	const float4 aoGather = ssao.Gather(defaultSampler, vertOutput.uv);
	const float ao = dot(aoGather, float4(1.0f)) / 4.0f;

	return lighting * ao;
}