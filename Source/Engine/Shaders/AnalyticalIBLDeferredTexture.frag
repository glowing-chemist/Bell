
#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float> depth;

[[vk::binding(2)]]
Texture2D<float3> vertexNormals;

[[vk::binding(3)]]
Texture2D<uint> materialIDTexture;

[[vk::binding(4)]]
Texture2D<float4> uvWithDerivitives;

[[vk::binding(5)]]
TextureCube<float4> skyBox

[[vk::binding(6)]]
TextureCube<float4> ConvolvedSkybox

[[vk::binding(7)]]
SamplerState linearSampler

#ifdef Shadow_Map
[[vk::binding(8)]]
Texture2D<float> shadowMap;
#endif

// an unbound array of material parameter textures
// In order albedo, normals, rougness, metalness
[[vk::binding(0, 1)]]
Texture2D materials[];

#define MATERIAL_COUNT 		4

float4 main(UVVertOutput vertOutput : TEXCOORD0)
{
    const float2 uv = vertOutput.uv;

	const float fragmentDepth = depth.Sample(linearSampler, uv);

	float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, float4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f));
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	const uint materialID = materialIDTexture.Sample(linearSampler, uv);

    float3 vertexNormal;
	vertexNormal = vertexNormals.Sample(linearSampler, uv);
    vertexNormal = remapNormals(vertexNormal);
    vertexNormal = normalize(vertexNormal);

	const float4 fragUVwithDifferentials = uvWithDerivitives.Sample(linearSampler, uv);

	const float2 xDerivities = unpackHalf2x16(floatBitsToUint(fragUVwithDifferentials.z));
    const float2 yDerivities = unpackHalf2x16(floatBitsToUint(fragUVwithDifferentials.w));

    const float3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const float4 baseAlbedo = materials[NonUniformResourceIndex(materialID * 4)].SampleGrad(  linearSampler, fragUVwithDifferentials.xy,
                                                                                    xDerivities, yDerivities);

    float3 normal = materials[NonUniformResourceIndex((materialID * 4) + 1)].Sample(linearSampler, fragUVwithDifferentials.xy);

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = materials[NonUniformResourceIndex((materialID * 4) + 2)].Sample(linearSampler, fragUVwithDifferentials.xy);

    const float metalness = materials[NonUniformResourceIndex((materialID * 4) + 3)].Sample(linearSampler, fragUVwithDifferentials.xy);

	{
    	float3x3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, float4(xDerivities, yDerivities));

    	normal = tbv * normal;

    	normal = normalize(normal);
	}

	const float3 lightDir = reflect(-viewDir, normal);

	const float lodLevel = roughness * 10.0f;

	float3 radiance = ConvolvedSkybox.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = skyBox.Sample(linearSampler, normal).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, uv);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

    const float cosTheta = dot(normal, viewDir);

    const float3 F = fresnelSchlickRoughness(max(cosTheta, 0.0), lerp(float3(DIELECTRIC_SPECULAR), baseAlbedo.xyz, metalness), roughness);

    const float remappedRoughness = pow(1.0f - roughness, 4.0f);
    const float3 FssEss = analyticalDFG(F, remappedRoughness, cosTheta);

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);

    return float4(FssEss * radiance + diffuse, 1.0);
}