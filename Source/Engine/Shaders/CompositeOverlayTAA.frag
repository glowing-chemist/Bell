#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

#if defined(Overlay)
#define USING_OVERLAY 1
#else
#define USING_OVERLAY 0
#endif

[[vk::binding(0)]]
Texture2D<float4> taaOutput;

#if defined(Overlay)
[[vk::binding(1)]]
Texture2D<float4> overlay;
#endif

[[vk::binding(1 + USING_OVERLAY)]]
SamplerState defaultSampler;

[[vk::binding(2 + USING_OVERLAY)]]
ConstantBuffer<CameraBuffer> camera;

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

#if USING_OVERLAY
	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);

	return ((1.0f - overlay.w) * colour) + overlay;
#else

	return colour;

#endif
}