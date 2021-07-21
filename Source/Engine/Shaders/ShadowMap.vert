#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "Skinning.hlsl"

[[vk::binding(0)]]
ConstantBuffer<ShadowingLight> light;

[[vk::binding(2)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(3)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


#if SHADE_FLAGS & ShadeFlag_Skinning
ShadowMapVertOutput main(SkinnedVertex vertex)
#else
ShadowMapVertOutput main(Vertex vertex)
#endif
{
	ShadowMapVertOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];
#if SHADE_FLAGS & ShadeFlag_Skinning
	const float4x4 skinningMatrix = calculateSkinningTransform(vertex, model.boneBufferOffset, skinningBones);

	float4 transformedPositionWS = mul(skinningMatrix, vertex.position);

	transformedPositionWS = float4(mul(transformedPositionWS, meshMatrix), 1.0f);
#else

	float4 transformedPositionWS = float4(mul(vertex.position, meshMatrix), 1.0f);

#endif

	output.position = mul(light.viewProj, transformedPositionWS);
	output.positionVS = mul(light.view, transformedPositionWS);
	output.uv = vertex.uv;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}