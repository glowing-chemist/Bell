#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "Skinning.hlsl"


[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(2)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::binding(3)]]
StructuredBuffer<float4x3> prevInstanceTransforms;

#if SHADE_FLAGS & ShadeFlag_Skinning
GBufferVertOutput main(SkinnedVertex vertex)
#else
GBufferVertOutput main(Vertex vertex)
#endif
{
	GBufferVertOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];
	float4x3 prevMeshMatrix = prevInstanceTransforms[model.transformsIndex];

	float3 normals = vertInput.normal.xyz;
	float3 tangents = vertInput.tangent.xyz;

#if SHADE_FLAGS & ShadeFlag_Skinning
	const float4x4 skinningMatrix = calculateSkinningTransform(vertex, model.boneBufferOffset, skinningBones);

	float4 transformedPositionWS = mul(skinningMatrix, vertex.position);
	float4 prevTransformedPositionWS = mul(skinningMatrix, vertex.position);

	transformedPositionWS = float4(mul(transformedPositionWS, meshMatrix), 1.0f);
	prevTransformedPositionWS = float4(mul(prevTransformedPositionWS, prevMeshMatrix), 1.0f);

	normals = mul((float3x3)skinningMatrix, normals);
	tangents = mul((float3x3)skinningMatrix, tangents);

#else

	float4 transformedPositionWS = float4(mul(vertex.position, meshMatrix), 1.0f);
	float4 prevTransformedPositionWS = float4(mul(vertex.position, prevMeshMatrix), 1.0f);

#endif

	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	float4 prevTransformedPosition = mul(camera.previousFrameViewProj, prevTransformedPositionWS);

	output.position = transformedPosition;
	output.curPosition = transformedPosition;
	output.prevPosition = prevTransformedPosition;
	output.positionWS = transformedPositionWS;
	output.normal = float4(normalize(mul(normals, (float3x3)meshMatrix)), 1.0f);
	output.tangent = float4(normalize(mul(tangents, (float3x3)meshMatrix)), vertex.tangent.w);
	output.colour = vertex.colour;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;
	output.uv = vertex.uv;

	return output;
}