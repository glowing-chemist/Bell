#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "PBR.hlsl"
#include "Utilities.hlsl"


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
#define START_MIP 7

#include "RayMarching.hlsl"


float4 main(PositionAndUVVertOutput vertInput)
{
	const float2 uv = vertInput.uv + camera.jitter;
	const float fragmentDepth = Depth.Sample(linearSampler, uv);

	if(fragmentDepth == 0.0f) // skybox.
		return float4(0.0f, 0.0f, 0.0f, 1.0f);

	float4 positionWS =  mul(camera.invertedViewProj, float4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f));
	positionWS /= positionWS.w;

	const float linDepth = lineariseReverseDepth(fragmentDepth, camera.nearPlane, camera.farPlane);
	float3 position = float3((uv - 0.5f) * 2.0f, linDepth);

	float3 view = normalize(camera.position - positionWS.xyz);

	const float roughness = 0.0f;//SpecularRoughness.Sample(linearSampler, uv).w;

	float3 normal = Normals.Sample(linearSampler, uv);
	normal = normalize(remapNormals(normal));

	uint width, height;
	LinearDepth.GetDimensions(width, height);
	width >>= START_MIP;
	height >>= START_MIP;

	float4 reflectedColour = float4(0.0f, 0.0f, 0.0f, 1.0f);
	float totalWeight = 0.0f;
	uint usedSamples = 0;
	for(uint i = 0; i < MAX_SAMPLE_COUNT; ++i)
	{
		const float2 Xi = Hammersley(i, MAX_SAMPLE_COUNT);
		const float3 H = ImportanceSampleGGX(Xi, roughness, normal);
		const float3 L = reflect(-view, H);

		const float max_distance = max(max(camera.sceneSize.x, camera.sceneSize.y), camera.sceneSize.z);
		float4 rayEnd = float4(positionWS + (L * max_distance), 1.0f);
		rayEnd = mul(camera.viewProj, rayEnd);
		rayEnd /= rayEnd.w;
		rayEnd.z = lineariseReverseDepth(rayEnd.z, camera.nearPlane, camera.farPlane);

		const float3 rayDirection = normalize(rayEnd.xyz - position.xyz);

		const float NoL = saturate(dot(normal, L));
		if(NoL > 0.0f)
		{
			// March the ray.
			const float2 colourUV = marchRay(position, rayDirection, 30, float2(1.0f, 1.0f) / float2(width, height), START_MIP);
			if(all(colourUV >= float2(0.0f, 0.0f)))
			{
				reflectedColour += diffuse.Sample(linearSampler, colourUV) * NoL;
				reflectedColour += float4(specular.Sample(linearSampler, colourUV).xyz, 1.0f) * NoL;
			}
			else
			{
				reflectedColour += skybox.SampleLevel(linearSampler, L, roughness * 10.0f) * NoL;
			}

			totalWeight += NoL;
			++usedSamples;
		}

		if(usedSamples >= SAMPLE_COUNT)
			break;
	}

	return reflectedColour / totalWeight;
}
