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
Texture2D<float4> SpecularRoughness;

[[vk::binding(4)]]
SamplerState linearSampler;

[[vk::binding(5)]]
ConstantBuffer<CameraBuffer> camera;

#if defined(Deferred_analytical_lighting)
[[vk::binding(6)]]
Texture2D<float> AnalyticalLighting;
#define USING_ANALYTICAL_LIGHTING
#endif


#define MAX_RAY_LENGTH 1.5f
#define SAMPLE_COUNT 5
#define MAX_SAMPLE_COUNT 15


float2 marchRay(float3 position, const float3 direction, const float rayLength, const uint maxSteps)
{

	const float3 finalPosition =  position + (direction * rayLength);

	for(uint i = 0; i < maxSteps; ++i)
	{
		const float3 steppedPosition = lerp(position, finalPosition, float(i + 1) / float(maxSteps));

		const float2 steppedUV = steppedPosition.xy * 0.5f + 0.5f;
		const float steppedDepth = LinearDepth.Sample(linearSampler, steppedUV);

		if(steppedPosition.z >= steppedDepth)
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
	normal = mul((float3x3)camera.viewProj, normal);
	normal = normalize(normal);

	const float4 specularRoughnes = SpecularRoughness.Sample(linearSampler, uv);
	float roughness = specularRoughnes.w;
	roughness *= roughness;

	// camera is at 0.0, 0.0, 0.0 so view vector is just the negative position sans the x component.
	const float3 view = float3(0.0f, -position.yz);
	float4 reflectedColour = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float totalWeight = 0.0f;
	uint usedSamples = 0;
	for(uint i = 0; i < MAX_SAMPLE_COUNT; ++i)
	{
		const float2 Xi = Hammersley(i, MAX_SAMPLE_COUNT);
		const float3 H = ImportanceSampleGGX(Xi, roughness, normal);
		float3 L = 2 * dot(view, H) * H - view;

		float NoL = saturate(dot(normal, L));
		if(NoL > 0.0)
		{
			// March the ray.
			const float2 colourUV = marchRay(position, L, MAX_RAY_LENGTH, 30);
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

	return (reflectedColour / totalWeight) * float4(specularRoughnes.xyz, 1.0f);
}
