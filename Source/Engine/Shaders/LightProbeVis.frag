#include "UniformBuffers.hlsl"
#include "LightProbes.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
StructuredBuffer<SphericalHarmonic> harmonics;


struct LightprobeVertOutput
{
	float4 position : SV_Position;
	float4 normal : NORMAL;
	uint index : PROBE_INDEX;
};


float4 main(LightprobeVertOutput input)
{
	const SphericalHarmonic harmonic = harmonics[input.index];

	return float4(calculateProbeIrradiance(input.normal.xyz, harmonic), 1.0f);
}