#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
Texture2D<float4> globalLighting;

[[vk::binding(1)]]
Texture2D<float4> analyticalLighting;

[[vk::binding(2)]]
Texture2D<float> ssao;

[[vk::binding(3)]]
Texture2D<float4> overlay;

[[vk::binding(4)]]
SamplerState defaultSampler;


float4 main(PositionAndUVVertOutput vertInput)
{
	const float4 globalLight = globalLighting.Sample(defaultSampler, vertInput.uv);
	const float4 analyticalLight = analyticalLighting.Sample(defaultSampler, vertInput.uv);
	const float4 lighting = globalLight + analyticalLight;
	const float4 aoGather = ssao.Sample(defaultSampler, vertInput.uv);
	const float ao = dot(aoGather, float4(1.0f)) / 4.0f;
	const float4 overlay = overlay.Sample(defaultSampler, vertInput.uv);

	return ((1.0f - overlay.w) * (lighting * ao)) + overlay;
}
