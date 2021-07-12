#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


DepthOnlyOutput main(Vertex vertex)
{
	DepthOnlyOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertex.position, meshMatrix);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	output.position = transformedPosition;
	output.uv = vertex.uv;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}