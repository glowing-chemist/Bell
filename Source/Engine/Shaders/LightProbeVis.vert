#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

struct Constants
{
	float4x4 transform;
	uint index;
};

[[vk::push_constant]]
ConstantBuffer<Constants> constants;


struct LightprobeVertOutput
{
	float4 position : SV_Position;
	float4 normal : NORMAL;
	uint index : PROBE_INDEX;
};

LightprobeVertOutput main(BasicVertex vertInput)
{
	LightprobeVertOutput output;
	output.index = constants.index;
	float4 transformedPositionWS = mul(constants.transform, vertInput.position);
	output.position = mul(camera.viewProj, transformedPositionWS);
	output.normal = vertInput.normal;

	return output;
}
