#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<ShadowingLight> light;

[[vk::push_constant]]
ConstantBuffer<ObjectMatracies> model;


ShadowMapVertOutput main(Vertex vert)
{
	ShadowMapVertOutput output;

	output.position = mul(mul(light.viewProj, model.meshMatrix), vert.position);
	output.positionVS = mul(mul(light.view, model.meshMatrix), vert.position);
	output.uv = vert.uv;
	output.materialID = vert.materialID;

	return output;
}