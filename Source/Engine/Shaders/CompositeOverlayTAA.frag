#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "ColourMapping.hlsl"

#if defined(Overlay)
#define USING_OVERLAY 1
#else
#define USING_OVERLAY 0
#endif

[[vk::binding(0)]]
Texture2D<float4> taaOutput;

#if USING_OVERLAY
[[vk::binding(1)]]
Texture2D<float4> overlay;
#endif

[[vk::binding(1 + USING_OVERLAY)]]
SamplerState defaultSampler;

[[vk::binding(2 + USING_OVERLAY)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::push_constant]]
ConstantBuffer<ColourCorrection> constants;

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

	colour = performColourMapping(colour, constants.gamma, constants.exposure);

#if USING_OVERLAY
	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);

	colour = ((1.0f - overlay.w) * colour) + overlay;
#endif

	return colour;
}