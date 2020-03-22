#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "PBR.glsl"
#include "NormalMapping.glsl"
#include "UniformBuffers.glsl"


#define MAX_MATERIALS 256


layout(location = 0) in float2 uv;


layout(location = 0) out float4 frameBuffer;

layout(set = 0, binding = 0) uniform cameraBuffer
{
	CameraBuffer camera;
};

layout(set = 0, binding = 1) uniform texture2D DFG;
layout(set = 0, binding = 2) uniform texture2D depth;
layout(set = 0, binding = 3) uniform texture2D vertexNormals;
layout(set = 0, binding = 4) uniform utexture2D materialIDTexture;
layout(set = 0, binding = 5) uniform texture2D uvWithDerivitives;
layout(set = 0, binding = 6) uniform textureCube skyBox;
layout(set = 0, binding = 7) uniform textureCube ConvolvedSkybox;
layout(set = 0, binding = 8) uniform sampler linearSampler;

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
layout(set = 1, binding = 0) uniform texture2D materials[];

#define MATERIAL_COUNT 		4

void main()
{
	const float fragmentDepth = texture(sampler2D(depth, linearSampler), uv).x;

	float4 worldSpaceFragmentPos = camera.invertedCamera * float4(uv, fragmentDepth, 1.0f);
	worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	const uint materialID = texture(usampler2D(materialIDTexture, linearSampler), uv).x;

	float3 vertexNormal;
    vertexNormal = texture(sampler2D(vertexNormals, linearSampler), uv).xyz;
    vertexNormal = remapNormals(vertexNormal);
    vertexNormal = normalize(vertexNormal);

	const float4 fragUVwithDifferentials = texture(sampler2D(uvWithDerivitives, linearSampler), uv);

	const float2 xDerivities = unpackHalf2x16(floatBitsToUint(fragUVwithDifferentials.z));
    const float2 yDerivities = unpackHalf2x16(floatBitsToUint(fragUVwithDifferentials.w));

    const float3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const float4 baseAlbedo = textureGrad(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler),
                                fragUVwithDifferentials.xy,
                                xDerivities,
                                yDerivities);

    float3 normal = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 1], linearSampler),
                                fragUVwithDifferentials.xy).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 2], linearSampler),
                                fragUVwithDifferentials.xy).x;

    const float metalness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 3], linearSampler),
                                fragUVwithDifferentials.xy).x;

	{
    	float3x3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, float4(xDerivities, yDerivities));

    	normal = tbv * normal;

    	normal = normalize(normal);
	}

	const float3 lightDir = reflect(-viewDir, normal);
    float NoV = dot(normal, viewDir);

	const float lodLevel = roughness * 10.0f;

	const float3 radiance = texture(samplerCube(ConvolvedSkybox, linearSampler), lightDir, lodLevel).xyz;

    const float3 irradiance = texture(samplerCube(skyBox, linearSampler), normal).xyz;

    const float2 f_ab = texture(sampler2D(DFG, linearSampler), float2(NoV, roughness)).xy;

    if(baseAlbedo.w == 0.0f)
        discard;

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const float3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    frameBuffer = float4(specular + diffuse, 1.0);
}