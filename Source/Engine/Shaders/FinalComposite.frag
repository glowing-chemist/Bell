#include "VertexOutputs.hlsl"

#if defined(Deferred_analytical_lighting)
#define USING_ANALYTICAL_LIGHTING 1
#else
#define USING_ANALYTICAL_LIGHTING 0
#endif

#if defined(Overlay)
#define USING_OVERLAY 1
#else
#define USING_OVERLAY 0
#endif

#if defined(SSAO) || defined(SSAO_Improved)
#define USING_SSAO 1
#else
#define USING_SSAO 0
#endif

#if (defined(Screen_Space_Reflection) || defined(RayTraced_Reflections)) && !defined(TAA)
#define USING_SSR 3
#else
#define USING_SSR 0
#endif


[[vk::binding(0)]]
Texture2D<float4> globalLighting;

#if defined(Deferred_analytical_lighting)
[[vk::binding(1)]]
Texture2D<float4> analyitcalLighting;
#endif

#if defined(Overlay)
[[vk::binding(USING_ANALYTICAL_LIGHTING + 1)]]
Texture2D<float4> overlay;
#endif

#if USING_SSAO
[[vk::binding(USING_OVERLAY + USING_ANALYTICAL_LIGHTING + 1)]]
Texture2D<float> ssao;
#endif

#if USING_SSR
[[vk::binding(USING_OVERLAY + USING_ANALYTICAL_LIGHTING + USING_SSAO + 1)]]
Texture2D<float4> reflectionMap;

[[vk::binding(USING_OVERLAY + USING_ANALYTICAL_LIGHTING + USING_SSAO + 2)]]
Texture2D<float4> specularRoughness;

[[vk::binding(USING_OVERLAY + USING_ANALYTICAL_LIGHTING + USING_SSAO + 3)]]
Texture2D<float2> reflectionUVs;
#endif

[[vk::binding(USING_OVERLAY + USING_ANALYTICAL_LIGHTING + USING_SSAO + USING_SSR + 1)]]
SamplerState defaultSampler;


float4 main(PositionAndUVVertOutput vertOutput)
{
	float4 lighting = globalLighting.Sample(defaultSampler, vertOutput.uv);

#if USING_SSR
	const float2 reflectionUV = reflectionUVs.Sample(defaultSampler, vertOutput.uv);
	if(all(reflectionUV >= float2(0.0f, 0.0f)))
	{
		const float3 specular = specularRoughness.Sample(defaultSampler, vertOutput.uv);
		const float3 relflection = reflectionMap.Sample(defaultSampler, vertOutput.uv).xyz;
		lighting.xyz = lerp(lighting.xyz, relflection * specular, specular);
	}
#endif

#if USING_ANALYTICAL_LIGHTING
	lighting += analyitcalLighting.Sample(defaultSampler, vertOutput.uv);
#endif

#if USING_SSAO
	lighting *= ssao.Sample(defaultSampler, vertOutput.uv);
#endif

	float4 result = lighting;
#if USING_OVERLAY
	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);
	result = ((1.0f - overlay.w) * lighting) + overlay;
#endif

	return result;
}
