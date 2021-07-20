#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "Skinning.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(3)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


#if SHADE_FLAGS & ShadeFlag_Skinning
DepthOnlyOutput main(SkinnedVertex vertInput)
#else
DepthOnlyOutput main(Vertex vertInput)
#endif
{
	DepthOnlyOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex];

#if SHADE_FLAGS & ShadeFlag_Skinning
	const float4x4 skinningMatrix = calculateSkinningTransform(vertInput, model.boneBufferOffset, skinningBones);

	float4 transformedPositionWS = mul(skinningMatrix, vertInput.position);

	transformedPositionWS = float4(mul(transformedPositionWS, meshMatrix), 1.0f);
#else

	float4 transformedPositionWS = float4(mul(vertInput.position, meshMatrix), 1.0f);

#endif

	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	output.position = transformedPosition;
	output.uv = vertInput.uv;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}