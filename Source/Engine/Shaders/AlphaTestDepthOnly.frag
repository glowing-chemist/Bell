#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];

void main(DepthOnlyOutput vertInput)
{
	// Just perform an alpha test.
	const float alpha = materials[vertInput.materialIndex].Sample(linearSampler, vertInput.uv).w;
	if(alpha == 0.0f)
		discard;
}