#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
Texture2D<float4> taaOutput;

[[vk::binding(1)]]
Texture2D<float4> overlay;

[[vk::binding(2)]]
SamplerState defaultSampler;

[[vk::binding(3)]]
ConstantBuffer<CameraBuffer> camera;

#if defined(Screen_Space_Reflection)
[[vk::binding(4)]]
Texture2D<float4> ReflectionMap;
#endif

float4 main(PositionAndUVVertOutput vertOutput)
{
	// Sharpen the taa output.
	// [ 0.0f, -1.0f, 0.0f
	//   -1.0f, 5.0f, -1.0f
	//   0.0f, -1.0f, 0.0f ]
	const float2 pixelSize = float2(1.0f, 1.0f) / camera.frameBufferSize;
	float4 colour = taaOutput.Sample(defaultSampler, vertOutput.uv);
	colour += colour * 4.0f;
	colour -= taaOutput.Sample(defaultSampler, vertOutput.uv + float2(0.0f, pixelSize.y));
	colour -= taaOutput.Sample(defaultSampler, vertOutput.uv + float2(pixelSize.x, 0.0f));
	colour -= taaOutput.Sample(defaultSampler, vertOutput.uv + float2(0.0f, -pixelSize.y));
	colour -= taaOutput.Sample(defaultSampler, vertOutput.uv + float2(-pixelSize.x, 0.0f));
	colour = saturate(colour);

#if defined(Screen_Space_Reflection)
	colour += ReflectionMap.Sample(defaultSampler, vertOutput.uv);
#endif

	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);

	return ((1.0f - overlay.w) * colour) + overlay;
}