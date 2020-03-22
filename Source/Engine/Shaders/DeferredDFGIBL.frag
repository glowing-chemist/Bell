
#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "ShadowMapping.hlsl"
#include "VertexOutputs.hlsl"

#define MAX_MATERIALS 256

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float2> DFG;

[[vk::binding(2)]]
Texture2D<float> depth;

[[vk::binding(3)]]
Texture2D<float3> vertexNormals;

[[vk::binding(4)]]
Texture2D<float4> Albedo;

[[vk::binding(5)]]
Texture2D<float2> MetalnessRoughness;

[[vk::binding(6)]]
TextureCube<float4> skyBox;

[[vk::binding(7)]]
TextureCube<float4> ConvolvedSkybox;

[[vk::binding(8)]]
SamplerState linearSampler;

#ifdef Shadow_Map
[[vk::binding(9)]]
Texture2D<float> shadowMap;
#endif


float4 main(UVVertOutput vertInput)
{
    const float2 uv = vertInput.uv;
	const float fragmentDepth = depth.Sample(linearSampler, uv);

	float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, float4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f));
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	float3 normal;
    normal = vertexNormals.Sample(linearSampler, uv);
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const float4 baseAlbedo = Albedo.Sample(linearSampler, uv);
    const float2 metalnessRoughness = MetalnessRoughness.Sample(linearSampler, uv);
    const float roughness = metalnessRoughness.y;
    const float metalness = metalnessRoughness.x;


	const float3 lightDir = reflect(-viewDir, normal);
    const float NoV = dot(normal, viewDir);

	const float lodLevel = roughness * 10.0f;

	float3 radiance = ConvolvedSkybox.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = skyBox.Sample(linearSampler, normal).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, uv);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

    const float2 f_ab = DFG.Sample(linearSampler, float2(NoV, roughness));

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const float3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    return float4(specular + diffuse, 1.0);
}