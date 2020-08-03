#include "VertexOutputs.hlsl"


struct UITranslation
{
	float4 translation;
};

[[vk::binding(0)]]
ConstantBuffer<UITranslation> trans : register(b0);


OverlayVertOutput main(OverlayVertex vert)
{
	OverlayVertOutput output;

	output.position = float4(vert.position * trans.translation.xy + trans.translation.zw, 0.0f, 1.0f);
	output.uv = vert.uv;
	output.albedo = vert.albedo;

	return output;
}
