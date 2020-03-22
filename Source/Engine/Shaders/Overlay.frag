#include "VertexOutputs.hlsl"

[[vk::binding(1)]]
Texture2D<float4> fontTexture;

[[vk::binding(2)]]
SamplerState linearSampler;

    
float4 main(OverlayVertOutput vertInput)
{
	float4 fonts = fontTexture.Sample(linearSampler, vertInput.uv);

	if(fonts.w == 0.0f)
		discard;

	return vertInput.albedo * fonts;
}
