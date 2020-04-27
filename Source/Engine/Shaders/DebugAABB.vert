#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

struct Transformation
{
	float4x4 AABBTrans;
};

[[vk::push_constant]]
ConstantBuffer<Transformation> trans;

PositionOutput main(float4 position : POSITION)
{
	float4 positionWS = mul(trans.AABBTrans, position);
	
	PositionOutput output;
	output.position = mul(camera.viewProj, positionWS);

	return output;	
}