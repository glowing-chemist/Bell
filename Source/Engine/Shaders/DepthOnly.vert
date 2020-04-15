#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


DepthOnlyOutput main(Vertex vertex)
{
	DepthOnlyOutput output;

	float4 transformedPositionWS = mul(model.meshMatrix, vertex.position.xyz);
	transformedPositionWS.w = 1.0f;
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	output.position = transformedPosition;
	output.uv = vertex.uv;
	output.materialID = model.materialID;

	return output;
}