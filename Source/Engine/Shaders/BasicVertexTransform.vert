#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

struct transformedPosition
{
	float4x4 trans;
};

[[vk::push_constant]]
ConstantBuffer<transformedPosition> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


PositionOutput main(Vertex vertex)
{
	PositionOutput output;

	float4 transformedPositionWS = mul(model.trans, vertex.position);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	
	return output;
}