#include "VertexOutputs.hlsl"

[[vk::binding(1)]]
Texture2D<float4> fontTexture : register(t0);

[[vk::binding(2)]]
SamplerState linearSampler : register(s0);

    
float4 main(OverlayVertOutput vertInput) : SV_Target
{
	float4 fonts = fontTexture.Sample(linearSampler, vertInput.uv);

	if(fonts.w == 0.0f)
		discard;

	return vertInput.albedo * fonts;
}
