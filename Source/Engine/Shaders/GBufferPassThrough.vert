#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"
#include "Skinning.hlsl"


[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(3)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::binding(4)]]
StructuredBuffer<float4x3> prevInstanceTransforms;

GBufferVertOutput main(Vertex vertInput, uint vertexID : SV_VertexID)
{
	GBufferVertOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];
	float4x3 prevMeshMatrix = prevInstanceTransforms[model.transformsIndex];
	float4 transformedPositionWS = float4(mul(vertInput.position, meshMatrix), 1.0f);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	float4 prevTransformedPositionWS = float4(mul(vertInput.position, prevMeshMatrix), 1.0f);
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