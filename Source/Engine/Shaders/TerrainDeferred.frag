#include "PBR.hlsl"
#include "MeshAttributes.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "VertexOutputs.hlsl"

struct GBufferFragOutput
{
    float4 diffuse;
    float2 normal;
    float4 specularRoughness;
    float2 velocity;
    float4 emissiveOcclusion;
};

[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(1)]]
SamplerState linearSampler;

[[vk::binding(0, 1)]]
Texture2D materials[];

[[vk::push_constant]]
ConstantBuffer<TerrainTextureing> constants;

#define MATERIAL_FLAGS kMaterial_Diffuse

#include "Materials.hlsl"

#define BLEND_SHARPNESS 1.0f

GBufferFragOutput main(TerrainVertexOutput vertex)
{
	const float3 viewDir = normalize(camera.position - vertex.worldPosition.xyz);
	const float2 uvx = vertex.worldPosition.zy / constants.textureScale;
	const float2 uvy = vertex.worldPosition.xz / constants.textureScale;
	const float2 uvz = vertex.worldPosition.xy / constants.textureScale;

	MaterialInfo matX = calculateMaterialInfo(float4(vertex.normal, 1.0f), float4(0.75f, 0.75f, 0.75f, 1.0f),
												constants.materialIndexXZ, float4(1.0f, 0.0f, 0.0f, 1.0f), viewDir, uvx);
	MaterialInfo matY = calculateMaterialInfo(float4(vertex.normal, 1.0f), float4(0.75f, 0.75f, 0.75f, 1.0f),
												constants.materialIndexY, float4(1.0f, 0.0f, 0.0f, 1.0f), viewDir, uvy);
	MaterialInfo matZ = calculateMaterialInfo(float4(vertex.normal, 1.0f), float4(0.75f, 0.75f, 0.75f, 1.0f),
												constants.materialIndexXZ, float4(1.0f, 0.0f, 0.0f, 1.0f), viewDir, uvz);

	float3 blendWeights = pow(abs(vertex.worldPosition.xyz), BLEND_SHARPNESS);
	blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

	const float2 velocity = (((vertex.clipPosition.xy / vertex.clipPosition.w) * 0.5f + 0.5f) - ((vertex.prevClipPosition.xy / vertex.prevClipPosition.w) * 0.5f + 0.5f));

	GBufferFragOutput output;
	output.diffuse = matX.diffuse * blendWeights.x + matY.diffuse * blendWeights.y + matZ.diffuse * blendWeights.z;
	output.normal = encodeOct(normalize(vertex.normal));
	output.specularRoughness = float4(0.0f, 0.0f, 0.0f, 1.0f);
	output.velocity = (velocity * 0.5f) + 0.5f;
	output.emissiveOcclusion = float4(0.0f, 0.0f, 0.0f, 1.0f);

	return output;
}