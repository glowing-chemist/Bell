#include "UniformBuffers.hlsl"
#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
TextureCube<float4> skyBox;

[[vk::binding(2)]]
SamplerState defaultSampler;

float4 main(PositionAndUVVertOutput vertInput)
{
	float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, float4((vertInput.uv - 0.5f) * 2.0f, 0.00001f, 1.0f));
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

    const float3 cubemapUV = normalize(worldSpaceFragmentPos.xyz - camera.position);

    return skyBox.Sample(defaultSampler, cubemapUV);
}