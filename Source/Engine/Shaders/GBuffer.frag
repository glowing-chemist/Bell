
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "VertexOutputs.hlsl"


struct GBufferFragOutput
{
    float4 albedo;
    float3 normal;
    float2 metalnessRoughness;
    float2 velocity;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];


GBufferFragOutput main(GBufferVertOutput vertInput)
{
    GBufferFragOutput output;

	const float4 baseAlbedo = materials[vertInput.materialID * 4].Sample(linearSampler, vertInput.uv);

    float3 normal = materials[vertInput.materialID * 4 + 1].Sample(linearSampler, vertInput.uv).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = materials[vertInput.materialID * 4 + 2].Sample(linearSampler, vertInput.uv).x;
    const float metalness = materials[vertInput.materialID * 4 + 3].Sample(linearSampler, vertInput.uv).x;

    const float2 xDerivities = ddx_fine(vertInput.uv);
    const float2 yDerivities = ddy_fine(vertInput.uv);

    const float3 viewDir = normalize(camera.position - vertInput.positionWS.xyz);

	{
    	float3x3 tbv = tangentSpaceMatrix(vertInput.normal, viewDir, float4(xDerivities, yDerivities));

    	normal = mul(normal, tbv);

    	normal = normalize(normal);
    }

    if(baseAlbedo.w == 0.0f)
        discard;

	output.albedo = baseAlbedo;
	output.normal = (normal + 1.0f) * 0.5f;
	output.metalnessRoughness = float2(metalness, roughness);
    output.velocity = (vertInput.velocity * 0.5f) + 0.5f;

    return output;
}