#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

#if defined(Overlay)
#define USING_OVERLAY 1
#else
#define USING_OVERLAY 0
#endif

#if defined(Screen_Space_Reflection) || defined(RayTraced_Reflections)
#define USING_SSR 3
#else
#define USING_SSR 0
#endif

[[vk::binding(0)]]
Texture2D<float4> taaOutput;

#if USING_OVERLAY
[[vk::binding(1)]]
Texture2D<float4> overlay;
#endif

#if USING_SSR
[[vk::binding(USING_OVERLAY  + 1)]]
Texture2D<float4> reflectionMap;

[[vk::binding(USING_OVERLAY + 2)]]
Texture2D<float4> specularRoughness;

[[vk::binding(USING_OVERLAY + 3)]]
Texture2D<float2> reflectionUVs;
#endif

[[vk::binding(1 + USING_OVERLAY + USING_SSR)]]
SamplerState defaultSampler;

[[vk::binding(2 + USING_OVERLAY + USING_SSR)]]
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

#if USING_SSR
	const float2 reflectionUV = reflectionUVs.Sample(defaultSampler, vertOutput.uv);
	if(all(reflectionUV != -1.0f))
	{
		const float3 specular = specularRoughness.Sample(defaultSampler, vertOutput.uv);
		const float3 relflection = reflectionMap.Sample(defaultSampler, vertOutput.uv).xyz;
		colour.xyz = lerp(colour.xyz, relflection * specular, specular);
	}
#endif

#if USING_OVERLAY
	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);

	return ((1.0f - overlay.w) * colour) + overlay;
#else

	return colour;

#endif
}