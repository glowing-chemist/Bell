#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "PBR.hlsl"


[[vk::binding(0)]]
Texture2D<float2> LinearDepth;

[[vk::binding(1)]]
Texture2D<float> Depth;

[[vk::binding(2)]]
Texture2D<float4> diffuse;

[[vk::binding(3)]]
Texture2D<float4> specular;

[[vk::binding(4)]]
Texture2D<float3> Normals;

[[vk::binding(5)]]
Texture2D<float4> SpecularRoughness;

[[vk::binding(6)]]
TextureCube<float4> skybox;

[[vk::binding(7)]]
SamplerState linearSampler;

[[vk::binding(8)]]
ConstantBuffer<CameraBuffer> camera;


#define SAMPLE_COUNT 5
#define MAX_SAMPLE_COUNT 15


#include "RayMarching.hlsl"


float4 main(PositionAndUVVertOutput vertInput)
{
	const float2 uv = vertInput.uv + camera.jitter;
	const float fragmentDepth = Depth.Sample(linearSampler, uv);

	if(fragmentDepth == 0.0f) // skybox.
		return float4(0.0f, 0.0f, 0.0f, 1.0f);

	float4 positionCS =  mul(camera.invertedPerspective, float4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f));
	positionCS /= positionCS.w;

	const float linDepth = LinearDepth.SampleLevel(linearSampler, uv, 0.0f).x;
	const float3 position = float3((uv - 0.5f) * 2.0f, linDepth);

	float3 view = normalize(-positionCS.xyz);
	view.z *= -1.0f;

	const float roughness = SpecularRoughness.Sample(linearSampler, uv).w;

	float3 normal = Normals.Sample(linearSampler, uv);
	normal = normalize(remapNormals(normal));
	normal = normalize(mul(float3x3(camera.view), normal));
	normal.z *= 1.0f;

	uint width, height;
	LinearDepth.GetDimensions(width, height);
	width >>= 7;
	height >>= 7;

	// camera is at 0.0, 0.0, 0.0 so view vector is just the negative position.
	float4 reflectedColour = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float totalWeight = 0.0f;
	uint usedSamples = 0;
	for(uint i = 0; i < MAX_SAMPLE_COUNT; ++i)
	{
		const float2 Xi = Hammersley(i, MAX_SAMPLE_COUNT);
		const float3 H = ImportanceSampleGGX(Xi, roughness, normal);
		float3 L = reflect(-view, H);
		L.z *= -1.0f; // bring in to same space as position.

		const float NoL = saturate(dot(normal, L));
		if(NoL > 0.0f)
		{
			// March the ray.
			const float2 colourUV = marchRay(position, L, 30, float2(1.0f, 1.0f) / float2(width, height), 7);
			if(all(colourUV >= float2(0.0f, 0.0f)))
			{
				reflectedColour += diffuse.Sample(linearSampler, colourUV) * NoL;
				reflectedColour += float4(specular.Sample(linearSampler, colourUV).xyz, 1.0f) * NoL;
			}
			else
			{
				const float3 LWS = mul(float3x3(camera.invertedView), L);
				reflectedColour += skybox.SampleLevel(linearSampler, LWS, roughness * 10.0f) * NoL;
			}

			totalWeight += NoL;
			++usedSamples;
		}

		if(usedSamples >= SAMPLE_COUNT)
			break;
	}

	return reflectedColour / totalWeight;
}
