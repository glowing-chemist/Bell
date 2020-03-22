#include "VertexOutputs.hlsl"

[[vk::binding(0, 0)]]
SamplerState linearSampler;

[[vk::binding(1, 1)]]
Texture2D materials[];

void main(ShadowMapVertOutput vertInput)
{
	// Just perform an alpha test.
	const float alpha = materials[vertInput.materialID].Sample(linearSampler, vertInput.uv);
	if(alpha == 0.0f)
		discard;
}