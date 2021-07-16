#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
StructuredBuffer<float4x4> skinningBones;

[[vk::binding(3)]]
StructuredBuffer<float4x3> instanceTransforms;

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;


DepthOnlyOutput main(Vertex vertex)
{
	DepthOnlyOutput output;

	float4x3 meshMatrix = instanceTransforms[model.transformsIndex]; 
	float4 transformedPositionWS = float4(mul(vertex.position, meshMatrix), 1.0f);
	float4 transformedPosition = mul(camera.viewProj, transformedPositionWS);
	output.position = transformedPosition;
	output.uv = vertex.uv;
	output.materialIndex = model.materialIndex;
	output.materialFlags = model.materialFlags;

	return output;
}