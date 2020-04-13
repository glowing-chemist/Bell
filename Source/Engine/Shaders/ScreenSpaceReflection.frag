#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "PBR.hlsl"


[[vk::binding(0)]]
Texture2D<float> LinearDepth;

[[vk::binding(1)]]
Texture2D<float4> GlobalLighting;

[[vk::binding(2)]]
Texture2D<float3> Normals;

[[vk::binding(3)]]
Texture2D<float2> MetalnessRoughnes;

[[vk::binding(4)]]
Texture2D<float4> Albedo;

[[vk::binding(5)]]
SamplerState linearSampler;

[[vk::binding(6)]]
ConstantBuffer<CameraBuffer> camera;

#if defined(Deferred_analytical_lighting)
[[vk::binding(7)]]
Texture2D<float> AnalyticalLighting;
#define USING_ANALYTICAL_LIGHTING
#endif


#define MAX_RAY_LENGTH 0.5f
#define SAMPLE_COUNT 5
#define MAX_SAMPLE_COUNT 15


float2 marchRay(float3 position, const float3 direction, const float rayLength, const uint maxSteps)
{
	const float xStepSize = (direction * rayLength).x / float(maxSteps);
	const float yStepSize = (direction * rayLength).y / float(maxSteps);
	float3 steppedPosition = position;

	for(uint i = 0; i < maxSteps; ++i)
	{
		steppedPosition.xy += float2(xStepSize, yStepSize);

		const float2 steppedUV = steppedPosition.xy * 0.5f + 0.5f;
		const float steppedDepth = LinearDepth.Sample(linearSampler, steppedUV);

		if(steppedDepth >= position.z)
			return steppedUV;
	}

	return float2(-1.0f, -1.0f);
}


float4 main(PositionAndUVVertOutput vertInput)
{
	const float2 uv = vertInput.uv + camera.jitter;
	const float fragmentDepth = LinearDepth.Sample(linearSampler, uv);

	float3 position = float3((uv - 0.5f) * 2.0f, fragmentDepth);

	float3 normal = Normals.Sample(linearSampler, uv);
	normal = remapNormals(normal);
	normal = normalize(normal);

	const float2 metalRoughnes = MetalnessRoughnes.Sample(linearSampler, uv);
	float roughness = metalRoughnes.y;
	roughness *= roughness;

	// camera is at 0.0, 0.0, 0.0 so view vector is just position.
	const float3 view = -position;
	float4 reflectedColour = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;
	uint usedSamples = 0;
	for(uint i = 0; i < MAX_SAMPLE_COUNT; ++i)
	{
		const float2 Xi = Hammersley(i, MAX_SAMPLE_COUNT);
		const float3 H = ImportanceSampleGGX(Xi, roughness, normal);
		const float3 L = 2 * dot(view, H) * H - view;

		float NoL = saturate(dot(normal, L));
		if(NoL > 0.0)
		{
			// March the ray.
			//prefilteredColor += skyBox.Sample(defaultSampler, L).xyz * NoL;
			const float2 colourUV = marchRay(position, normalize(L), MAX_RAY_LENGTH, 30);
			if(all(colourUV >= float2(0.0f, 0.0f)))
			{
				reflectedColour += GlobalLighting.Sample(linearSampler, colourUV) * NoL;

#if defined(Deferred_analytical_lighting)
				reflectedColour += AnalyticalLighting.Sample(linearSampler, colourUV) * NoL;
#endif

				totalWeight += NoL;
			}

			++usedSamples;
		}

		if(usedSamples >= SAMPLE_COUNT)
			break;
	}

	const float3 albedo = Albedo.Sample(linearSampler, uv);
	const float NoV = dot(normal, view);
	const float3 F0 = lerp(float3(DIELECTRIC_SPECULAR), albedo, metalRoughnes.x);
    const float3 F = fresnelSchlickRoughness(max(NoV, 0.0), F0, roughness);
	return (reflectedColour / totalWeight) * float4(F, 1.0f);
}
