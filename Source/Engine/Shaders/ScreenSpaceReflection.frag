#include "VertexOutputs.hlsl"
#include "NormalMapping.hlsl"
#include "PBR.hlsl"


[[vk::binding(0)]]
Texture2D<float> linearDepth;

[[vk::binding(1)]]
Texture2D<float4> globalLighting;

[[vk::binding(2)]]
Texture2D<float3> normals;

[[vk::binding(3)]]
Texture2D<float2> metalnessRoughnes;

[[vk::binding(4)]]
SamplerState linearSampler;

#if defined(Deferred_analytical_lighting)
[[vk::binding(5)]]
Texture2D<float> analyticalLighting;
#define USING_ANALYTICAL_LIGHTING
#endif


#define MAX_RAY_LENGTH 0.25f
#define RAY_STEP_SIZE 0.05f
#define SAMPLE_COUNT 5


float2 marchRay(float3 position, const float3 direction, const float rayLength, const uint maxSteps, Texture2D<float> depth, SamplerState samp)
{
	const float xStepSize = (direction * rayLength).x / float(maxSteps);
	const float yStepSize = (direction * rayLength).y / float(maxSteps);
	float3 steppedPosition = position;

	for(uint i = 0; i < maxSteps; ++i)
	{
		steppedPosition.xy += float2(xStepSize, yStepSize);

		const float2 steppedUV = steppedPosition.xy * 0.5f + 0.5f;
		const float steppedDepth = depth.Sample(samp, steppedUV);

		if(steppedDepth >= position.z)
			return steppedUV;
	}

	return float2(-1.0f, -1.0f);
}


float4 main(PositionAndUVVertOutput vertInput)
{
	const float fragmentDepth = linearDepth.Sample(linearSampler, vertInput.uv);

	float3 position = float3((vertInput.uv - 0.5f) * 2.0f, fragmentDepth);

	float3 normal = normals.Sample(linearSampler, vertInput.uv);
	normal = remapNormals(normal);
	normal = normalize(normal);

	float roughness = metalnessRoughnes.Sample(linearSampler, vertInput.uv).y;
	roughness *= roughness;

	// camera is at 0.0, 0.0, 0.0 so view vector is just position.
	const float3 view = position;
	float4 reflectedColour = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;
	for(uint i = 0; i < SAMPLE_COUNT; ++i)
	{
		const float2 Xi = Hammersley(i, SAMPLE_COUNT);
		const float3 H = ImportanceSampleGGX(Xi, roughness, normal);
		const float3 L = 2 * dot(view, H) * H - view;

		float NoL = saturate(dot(normal, L));
		if(NoL > 0.0)
		{
			// March the ray.
			//prefilteredColor += skyBox.Sample(defaultSampler, L).xyz * NoL;
			const float2 colourUV = marchRay(position, normalize(L), MAX_RAY_LENGTH, 15, linearDepth, linearSampler);
			if(all(colourUV >= float2(0.0f, 0.0f)))
			{
				reflectedColour += globalLighting.Sample(linearSampler, colourUV) * NoL;

#if defined(Deferred_analytical_lighting)
				reflectedColour += analyticalLighting.Sample(linearSampler, colourUV) * NoL;
#endif

				totalWeight += NoL;
			}
		}
	}

	return reflectedColour / totalWeight;
}
