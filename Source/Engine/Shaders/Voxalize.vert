#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;


GBufferVertOutput main(Vertex vertex)
{
	GBufferVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertex.position, meshMatrix);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);

	output.position = transformedPosition;
	output.positionWS = transformedPositionWS;
	output.normal = float4(mul(vertex.normal.xyz,(float3x3)meshMatrix), 1.0f);
	output.tangent = float4(mul(vertex.tangent.xyz,(float3x3)meshMatrix),vertex.tangent.w);
	output.colour = vertex.colour;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;
	output.uv = vertex.uv;

	return output;
}