#include "UniformBuffers.hlsl"
#include "LightProbes.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


struct LightprobeVertOutput
{
	float4 position : SV_Position;
	float4 normal : NORMAL;
	uint index : PROBE_INDEX;
};


float4 main(LightprobeVertOutput input)
{
	return float4(0.75f, 0.75f, 0.75f, 1.0f);
}