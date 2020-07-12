#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "ShadowMapping.hlsl"
#include "VertexOutputs.hlsl"
#include "LightProbes.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float3> DFG;

[[vk::binding(2)]]
Texture2D<float> depth;

[[vk::binding(3)]]
Texture2D<float3> Normals;

[[vk::binding(4)]]
Texture2D<float4> Diffuse;

[[vk::binding(5)]]
Texture2D<float4> SpecularRoughness;

[[vk::binding(6)]]
Texture2D<float4> EmissiveOcclusion;

[[vk::binding(7)]]
TextureCube<float4> ConvolvedSkyboxSpecular;

[[vk::binding(8)]]
SamplerState linearSampler;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map)
[[vk::binding(9)]]
Texture2D<float> shadowMap;
#endif

[[vk::binding(0, 1)]]
StructuredBuffer<SphericalHarmonic> harmonics;

[[vk::binding(1, 1)]]
Texture3D<int4> ProbeLookupTable;


float4 main(UVVertOutput vertInput)
{
    const float2 uv = vertInput.uv;
	const float fragmentDepth = depth.Sample(linearSampler, uv);

	float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, float4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f));
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	float3 normal;
    normal = Normals.Sample(linearSampler, uv);
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const float4 diffuse = Diffuse.Sample(linearSampler, uv);
    const float4 specularRoughness = SpecularRoughness.Sample(linearSampler, uv);
    const float roughness = specularRoughness.w;
    const float4 emissiveOcclusion = EmissiveOcclusion.Sample(linearSampler, uv);

	const float3 lightDir = reflect(-viewDir, normal);
    const float NoV = dot(normal, viewDir);

	const float lodLevel = roughness * 10.0f;

	float3 radiance = ConvolvedSkyboxSpecular.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance;
    {
        const float3 lookupSamplePos = worldSpaceFragmentPos.xyz / camera.sceneSize.xyz;
        const uint4 harmonicsIndicies = ProbeLookupTable.Sample(linearSampler, lookupSamplePos);

        SphericalHarmonic probe1 = harmonics[harmonicsIndicies.x];
        SphericalHarmonic probe2 = harmonics[harmonicsIndicies.y];
        SphericalHarmonic probe3 = harmonics[harmonicsIndicies.z];

        irradiance = calculateProbeIrradiance(worldSpaceFragmentPos.xyz, normal, probe1, probe2, probe3);
    }

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map)
    const float occlusion = shadowMap.Sample(linearSampler, uv);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

    const float3 dfg = DFG.Sample(linearSampler, float2(NoV, roughness * roughness));

    MaterialInfo material;
    material.diffuse = diffuse;
    material.normal = float4(normal, 1.0f);
    material.specularRoughness = specularRoughness;
    const float3 diffuseLighting = calculateDiffuseDisney(material, irradiance, dfg);
    const float3 specularlighting = calculateSpecular(material, radiance, dfg);

    return float4(specularlighting + diffuseLighting + emissiveOcclusion.xyz, 1.0f) * emissiveOcclusion.w;
}