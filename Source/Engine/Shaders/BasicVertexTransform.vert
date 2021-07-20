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


#if SHADE_FLAGS & ShadeFlag_Skinning
PositionOutput main(SkinnedVertex vertInput)
#else
PositionOutput main(Vertex vertInput)
#endif
{
	PositionOutput output;

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
	
	return output;
}