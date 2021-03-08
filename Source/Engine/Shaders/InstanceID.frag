#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(1)]]
RWStructuredBuffer<uint> instanceID;

struct InstanceInfo
{
	float4x4 trans;
	uint id;
};

[[vk::push_constant]]
ConstantBuffer<InstanceInfo> model;

uint main(InstanceIDOutput vertInput)
{
	return vertInput.id;
}