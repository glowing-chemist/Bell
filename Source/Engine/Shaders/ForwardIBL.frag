#include "VertexOutputs.hlsl"
#include "MeshAttributes.hlsl"
#include "PBR.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"


struct Output
{
    float4 colour;
    float velocity;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
Texture2D<float2> DFG;

[[vk::binding(2)]]
TextureCube<float4> skyBox;

[[vk::binding(3)]]
TextureCube<float4> ConvolvedSkybox;

[[vk::binding(4)]]
SamplerState linearSampler;

#ifdef Shadow_Map
[[vk::binding(5)]]
Texture2D<float> shadowMap;
#endif

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
[[vk::binding(0, 1)]]
Texture2D materials[];


Output main(GBufferVertOutput vertInput)
{
	const float4 baseAlbedo = materials[vertInput.materialID * 4].Sample(linearSampler, vertInput.uv);

    float3 normal = materials[(vertInput.materialID * 4) + 1].Sample(linearSampler, vertInput.uv).xyz;
    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = materials[(vertInput.materialID * 4) + 2].Sample(linearSampler, vertInput.uv).x;

    const float metalness = materials[(vertInput.materialID * 4) + 3].Sample(linearSampler, vertInput.uv);

	const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

	const float2 xDerivities = ddx_fine(vertInput.uv);
	const float2 yDerivities = ddy_fine(vertInput.uv);

	{
    	float3x3 tbv = tangentSpaceMatrix(vertInput.normal, viewDir, float4(xDerivities, yDerivities));

    	normal = mul(normal, tbv);

    	normal = normalize(normal);
	}

	const float3 lightDir = reflect(-viewDir, normal);

	const float lodLevel = roughness * 10.0f;

	float3 radiance = ConvolvedSkybox.SampleLevel(linearSampler, lightDir, lodLevel).xyz;

    float3 irradiance = skyBox.Sample(linearSampler, normal).xyz;

#ifdef Shadow_Map
    const float occlusion = shadowMap.Sample(linearSampler, vertInput.position.xy / camera.frameBufferSize);
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(normal, viewDir);
    const float2 f_ab = DFG.Sample(linearSampler, float2(NoV, roughness));

    if(baseAlbedo.w == 0.0f)
        discard;

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const float3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    Output output;

    output.colour = float4(specular + diffuse, 1.0);
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}