#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];

[[vk::push_constant]]
ConstantBuffer<MeshInstanceInfo> model;

float2 main(ShadowMapVertOutput vertInput): SV_Target0
{
	if(model.materialFlags & kAlphaTested)
	{
		const float alpha = materials[vertInput.materialIndex].Sample(linearSampler, vertInput.uv).w;
		if(alpha == 0.0f)
			discard;
	}
	
	float2 shadowMap;
	shadowMap.x = -vertInput.positionVS.z;
	shadowMap.y = vertInput.positionVS.z * vertInput.positionVS.z;

	return shadowMap;
}
