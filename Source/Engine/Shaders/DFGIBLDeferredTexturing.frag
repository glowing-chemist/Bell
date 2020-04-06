#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
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
Texture2D<uint> materialIDTexture;

[[vk::binding(5)]]
Texture2D<float4> uvWithDerivitives;

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

// an unbound array of material parameter textures
// In order albedo, normals, rougness, metalness
[[vk::binding(0, 1)]]
Texture2D materials[];

#define MATERIAL_COUNT 		4

float4 main(PositionAndUVVertOutput vertOutput)
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

    const float2 xDerivities = float2(asfloat(asuint(fragUVwithDifferentials.z) & 0xFFFF0000), asfloat(asuint(fragUVwithDifferentials.z) >> 16));
    const float2 yDerivities = float2(asfloat(asuint(fragUVwithDifferentials.w) & 0xFFFF0000), asfloat(asuint(fragUVwithDifferentials.w) >> 16));

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
    float NoV = dot(normal, viewDir);

	const float lodLevel = roughness * 10.0f;

    float3 radiance = ConvolvedSkybox.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = skyBox.Sample(linearSampler, normal).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, uv);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

    const float2 f_ab = DFG.Sample(linearSampler, float2(NoV, roughness));

    if(baseAlbedo.w == 0.0f)
        discard;

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const float3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    return float4(specular + diffuse, 1.0);
}