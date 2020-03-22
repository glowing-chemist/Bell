#include "VertexOutputs.hlsl"

PositionAndUVVertOutput main(uint vertID : SV_VertexID)
{
	PositionAndUVVertOutput output;

	output.uv = float2((vertID << 1) & 2, vertID & 2);
	output.position = float4(output.uv * 2.0f + -1.0f, 0.0f, 1.0f);

	return output;
}
