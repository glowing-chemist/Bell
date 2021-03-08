#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

struct InstanceInfo
{
	float4x4 trans;
	uint id;
};

[[vk::push_constant]]
ConstantBuffer<InstanceInfo> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


InstanceIDOutput main(Vertex vertex)
{
	InstanceIDOutput output;

	float4 transformedPositionWS = mul(model.trans, vertex.position);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	output.id = model.id;
	
	return output;
}