#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertex)
{
	GBufferVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertex.position.xyz, meshMatrix);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	output.positionWS = transformedPositionWS;
	output.normal = float4(mul(vertex.normal.xyz,(float3x3)meshMatrix), 1.0f);
	output.materialIndex = model.materialIndex;
	output.materailFlags = model.materailFlags;
	output.uv = vertex.uv;

	return output;
}