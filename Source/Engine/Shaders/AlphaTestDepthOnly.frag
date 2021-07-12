#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "PBR.hlsl"

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;

void main(DepthOnlyOutput vertInput)
{
	if(model.materialFlags & kMaterial_AlphaTested)
	{
		// Just perform an alpha test.
		const float alpha = materials[vertInput.materialIndex].Sample(linearSampler, vertInput.uv).w;
		if(alpha == 0.0f)
			discard;
	}
}