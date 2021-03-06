#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "ShadowMapping.hlsl"
#include "VertexOutputs.hlsl"
#include "LightProbes.hlsl"
#include "KdTree.hlsl"

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float3> DFG;

[[vk::binding(2)]]
Texture2D<float> depth;

[[vk::binding(3)]]
Texture2D<float2> Normals;

[[vk::binding(4)]]
Texture2D<float4> Diffuse;

[[vk::binding(5)]]
Texture2D<float4> SpecularRoughness;

[[vk::binding(6)]]
Texture2D<float4> EmissiveOcclusion;

#if defined(Screen_Space_Reflection) || defined(RayTraced_Reflections)
[[vk::binding(7)]]
Texture2D<float4> reflectionMap;
#else
[[vk::binding(7)]]
TextureCube<float4> ConvolvedSkyboxSpecular;
#endif

[[vk::binding(8)]]
SamplerState linearSampler;

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map) || defined(RayTraced_Shadows)
[[vk::binding(9)]]
Texture2D<float> shadowMap;
#endif

[[vk::binding(0, 1)]]
StructuredBuffer<SphericalHarmonic> harmonics;

[[vk::binding(1, 1)]]
StructuredBuffer<KdNode> irradianceProbeTree;


float4 main(UVVertOutput vertInput)
{
    const float2 uv = vertInput.uv;
    const uint2 pixel = vertInput.uv * camera.frameBufferSize;
	const float fragmentDepth = depth.Sample(linearSampler, uv);

    if(fragmentDepth == 0.0f) // skybox
        return float4(0.0f, 0.0f, 0.0f, 1.0f);

	float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, float4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f));
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	float3 normal = decodeOct(Normals.Load(int3(pixel, 0)));

    const float3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const float4 diffuse = Diffuse.Sample(linearSampler, uv);
    const float4 specularRoughness = SpecularRoughness.Sample(linearSampler, uv);
    const float roughness = specularRoughness.w;
    const float4 emissiveOcclusion = EmissiveOcclusion.Sample(linearSampler, uv);

	const float3 lightDir = reflect(-viewDir, normal);
    const float NoV = dot(normal, viewDir);

#if defined(Screen_Space_Reflection) || defined(RayTraced_Reflections)
    float3 radiance = reflectionMap.Sample(linearSampler, uv);
#else
    const float lodLevel = roughness * 10.0f;

    float3 radiance = ConvolvedSkyboxSpecular.SampleLevel(linearSampler, lightDir, lodLevel).xyz;
#endif

    float3 irradiance;
    {
        // TODO extend to lookup multiple probes and interpolate.
        const float3 lookupSamplePos = worldSpaceFragmentPos.xyz / camera.sceneSize.xyz;
        const uint harmonicsIndicies = findNearestPoint(irradianceProbeTree, lookupSamplePos);

        SphericalHarmonic probe1 = harmonics[harmonicsIndicies];

        irradiance = calculateProbeIrradiance(normal, probe1);
    }

#if defined(Shadow_Map) || defined(Cascade_Shadow_Map) || defined(RayTraced_Shadows)
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