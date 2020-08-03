#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
Texture2D<float4> tex;

[[vk::binding(1)]]
SamplerState defaultSampler;

float4 main(PositionAndUVVertOutput vertInput) : SV_Target
{
	return  tex.Sample(defaultSampler, vertInput.uv);
}
