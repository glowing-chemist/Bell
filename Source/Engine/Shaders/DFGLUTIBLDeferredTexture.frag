#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "PBR.glsl"
#include "NormalMapping.glsl"
#include "UniformBuffers.glsl"


#define MAX_MATERIALS 256


layout(location = 0) in vec2 uv;


layout(location = 0) out vec4 frameBuffer;

layout(binding = 0) uniform cameraBuffer
{
	CameraBuffer camera;
};

layout(binding = 1) uniform texture2D DFG;
layout(binding = 2) uniform texture2D depth;
layout(binding = 3) uniform texture2D vertexNormals;
layout(binding = 4) uniform utexture2D materialIDTexture;
layout(binding = 5) uniform texture2D uvWithDerivitives;
layout(binding = 6) uniform textureCube skyBox;
layout(binding = 7) uniform textureCube ConvolvedSkybox;
layout(binding = 8) uniform sampler linearSampler;

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
layout(binding = 9) uniform texture2D materials[];

#define MATERIAL_COUNT 		4

void main()
{
	const float fragmentDepth = texture(sampler2D(depth, linearSampler), uv).x;

	vec4 worldSpaceFragmentPos = camera.invertedCamera * vec4(uv, fragmentDepth, 1.0f);
	worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	const uint materialID = texture(usampler2D(materialIDTexture, linearSampler), uv).x;

	vec3 vertexNormal = texture(sampler2D(vertexNormals, linearSampler), uv).xyz;
	vertexNormal = normalize(remapNormals(vertexNormal));

	const vec4 fragUVwithDifferentials = texture(sampler2D(uvWithDerivitives, linearSampler), uv);

	const vec2 xDerivities = unpackHalf2x16(floatBitsToUint(fragUVwithDifferentials.z));
    const vec2 yDerivities = unpackHalf2x16(floatBitsToUint(fragUVwithDifferentials.w));

    const vec3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const vec3 baseAlbedo = textureGrad(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler),
                                fragUVwithDifferentials.xy,
                                xDerivities,
                                yDerivities).xyz;

    vec3 normal = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 1], linearSampler),
                                fragUVwithDifferentials.xy).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 2], linearSampler),
                                fragUVwithDifferentials.xy).x;

    const float metalness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 3], linearSampler),
                                fragUVwithDifferentials.xy).x;

	{
    	mat3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, vec4(xDerivities, yDerivities));

    	normal = tbv * normal;

    	normal = normalize(normal);
	}

	const vec3 lightDir = reflect(-viewDir, normal);
    float NoV = dot(normal, viewDir);

	const float lodLevel = roughness * 10.0f;

	const vec3 radiance = texture(samplerCube(ConvolvedSkybox, linearSampler), lightDir, lodLevel).xyz;

    const vec3 irradiance = texture(samplerCube(skyBox, linearSampler), normal).xyz;

    const vec2 f_ab = texture(sampler2D(DFG, linearSampler), vec2(NoV, roughness)).xy;

    const vec3 diffuse = calculateDiffuseGlobalIBL(baseAlbedo.xyz, metalness, irradiance);
    const vec3 specular = calculateSpecularGlobalIBL(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    frameBuffer = vec4(specular + diffuse, 1.0);
}