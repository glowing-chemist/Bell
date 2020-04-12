#include "VertexOutputs.hlsl"

[[vk::binding(0)]]
Texture2D<float4> globalLighting;

#if defined(Deferred_analytical_lighting)
[[vk::binding(1)]]
Texture2D<float4> analyitcalLighting;
#define ANALYTIC_BINDING 1
#endif

#if defined(Overlay)
#if defined(Deferred_analytical_lighting)
[[vk::binding(2)]]
#else
[[vk::binding(1)]]
#endif
Texture2D<float4> overlay;
#endif

#if defined(SSAO) || defined(SSAO_Improved)
#if defined(Deferred_analytical_lighting) && defined(Overlay)
[[vk::binding(3)]]
#elif defined(Deferred_analytical_lighting) || defined(Overlay)
[[vk::binding(2)]]
#else
[[vk::binding(1)]]
#endif
Texture2D<float> ssao;
#endif

#if (defined(SSAO) || defined(SSAO_Improved)) && defined(Deferred_analytical_lighting) && defined(Overlay)
[[vk::binding(4)]]
#elif (defined(Deferred_analytical_lighting) && defined(Overlay)) || ((defined(SSAO) || defined(SSAO_Improved)) && defined(Overlay)) || ((defined(SSAO) || defined(SSAO_Improved)) && defined(Deferred_analytical_lighting))
[[vk::binding(3)]]
#elif defined(Deferred_analytical_lighting) || defined(Overlay) || (defined(SSAO) || defined(SSAO_Improved))
[[vk::binding(2)]]
#else
[[vk::binding(1)]]
#endif
SamplerState defaultSampler;


float4 main(PositionAndUVVertOutput vertOutput)
{
	float4 lighting = globalLighting.Sample(defaultSampler, vertOutput.uv);

#if defined(Deferred_analytical_lighting)
	lighting += analyitcalLighting.Sample(defaultSampler, vertOutput.uv);
#endif

#if defined(SSAO) || defined(SSAO_Improved)
	lighting *= ssao.Sample(defaultSampler, vertOutput.uv);
#endif

	float4 result = lighting;
#if defined(Overlay)
	const float4 overlay = overlay.Sample(defaultSampler, vertOutput.uv);
	result = ((1.0f - overlay.w) * lighting) + overlay;
#endif

	return result;
}