
#include "VertexOutputs.hlsl"
#include "ClusteredLighting.hlsl"

[[vk::binding(1, 1)]]
StructuredBuffer<Light> sceneLights;


struct transformedPosition
{
	float4x4 trans;
	uint lightIndex;
};

[[vk::push_constant]]
ConstantBuffer<transformedPosition> light;


float4 main(PositionOutput input) : SV_Target0
{
	return sceneLights[light.lightIndex].albedo;
}