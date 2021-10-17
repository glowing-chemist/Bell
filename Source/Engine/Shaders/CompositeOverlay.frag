#include "VertexOutputs.hlsl"
#include "ColourMapping.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
Texture2D<float4> globalLighting;

[[vk::binding(1)]]
Texture2D<float4> overlay;

[[vk::binding(2)]]
SamplerState defaultSampler;

[[vk::push_constant]]
ConstantBuffer<ColourCorrection> constants;

float4 main(PositionAndUVVertOutput vertOutput) : SV_Target0
{
	float4 lighting = globalLighting.Sample(defaultSampler, vertOutput.uv);
	const float4 overlaySample = overlay.Sample(defaultSampler, vertOutput.uv);

#ifndef TAA
	lighting = performColourMapping(lighting, constants.gamma, constants.exposure);
#endif

	return ((1.0f - overlaySample.w) * lighting) + overlaySample;
}
