#include "VertexOutputs.hlsl"
#include "ColourMapping.hlsl"

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

#if defined(SSAO)
#define USING_SSAO 1
#else
#define USING_SSAO 0
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

[[vk::binding(USING_OVERLAY + USING_ANALYTICAL_LIGHTING + USING_SSAO + 1)]]
SamplerState defaultSampler;


float4 main(PositionAndUVVertOutput vertOutput)
{
	float4 lighting = globalLighting.Sample(defaultSampler, vertOutput.uv);

#if USING_ANALYTICAL_LIGHTING
	lighting += analyitcalLighting.Sample(defaultSampler, vertOutput.uv);
#endif

#if USING_SSAO
	lighting *= ssao.Sample(defaultSampler, vertOutput.uv);
#endif

#if defined(TAA)
	float4 result = lighting;
#else
	float4 result = performColourMapping(lighting, 2.2f, 1.0f);
#endif

#if USING_OVERLAY
	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);
	result = ((1.0f - overlay.w) * result) + overlay;
#endif

	return result;
}
