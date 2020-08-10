#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
TextureCube<float4> skyBox;

[[vk::binding(2)]]
SamplerState defaultSampler;

struct Output
{
	float4 colour;
	float2 velocity;
};

Output main(PositionAndUVVertOutput vertInput)
{
	const float4 positionNDC = float4((vertInput.uv - 0.5f) * 2.0f, 0.00001f, 1.0f); 
	float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, positionNDC);
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

    const float3 cubemapUV = normalize(worldSpaceFragmentPos.xyz - camera.position);

    float4 previousPositoinNDC = mul(camera.previousFrameViewProj, worldSpaceFragmentPos);
    previousPositoinNDC /= previousPositoinNDC.w;

    Output output;
    output.colour = skyBox.Sample(defaultSampler, cubemapUV);
    output.velocity = 0.5f * (positionNDC.xy - previousPositoinNDC.xy) + 0.5f;

    return output;
}