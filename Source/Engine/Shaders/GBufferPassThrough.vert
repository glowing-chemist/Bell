#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"
#include "Skinning.hlsl"


[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<float4x4> skinningBones;

GBufferVertOutput main(Vertex vertInput, uint vertexID : SV_VertexID)
{
	GBufferVertOutput output;

	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
	recreateMeshMatracies(model.meshMatrix, model.prevMeshMatrix, meshMatrix, prevMeshMatrix);
	float4 transformedPositionWS = mul(vertInput.position, meshMatrix);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	float4 prevTransformedPositionWS = mul(vertInput.position, prevMeshMatrix);
	float4 prevTransformedPosition = mul(camera.previousFrameViewProj, prevTransformedPositionWS);

#if 0 //SHADE_FLAGS & ShadeFlag_Skinning
	{


		const float4x4 skinningMatrix = calculateSkinningTransform(vertexID, model.boneBufferOffset, perVertexSkinningBuffers[model.boneCountBufferIndex], perVertexSkinningBuffers[model.boneWeightBufferIndex]);
		meshMatrix = mul(meshMatrix, skinningMatrix);
		prevMeshMatrix = mul(prevMeshMatrix, skinningMatrix); // TODO calculate prev skinning matracies.
	}
}
#endif

	output.position = transformedPosition;
	output.curPosition = transformedPosition;
	output.prevPosition = prevTransformedPosition;
	output.positionWS = transformedPositionWS;
	output.uv = vertInput.uv;
	output.normal = float4(normalize(mul(vertInput.normal.xyz, (float3x3)meshMatrix)), 1.0f);
	output.tangent = float4(normalize(mul(vertInput.tangent.xyz, (float3x3)meshMatrix)), vertInput.tangent.w);
	output.colour = vertInput.colour;
	output.materialIndex =  model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}