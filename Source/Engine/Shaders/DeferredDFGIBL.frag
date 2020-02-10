#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "PBR.glsl"
#include "NormalMapping.glsl"
#include "UniformBuffers.glsl"


#define MAX_MATERIALS 256


layout(location = 0) in vec2 uv;


layout(location = 0) out vec4 frameBuffer;

layout(set = 0, binding = 0) uniform cameraBuffer
{
	CameraBuffer camera;
};

layout(set = 0, binding = 1) uniform texture2D DFG;
layout(set = 0, binding = 2) uniform texture2D depth;
layout(set = 0, binding = 3) uniform texture2D vertexNormals;
layout(set = 0, binding = 4) uniform texture2D Albedo;
layout(set = 0, binding = 5) uniform texture2D MetalnessRoughness;
layout(set = 0, binding = 6) uniform textureCube skyBox;
layout(set = 0, binding = 7) uniform textureCube ConvolvedSkybox;
layout(set = 0, binding = 8) uniform sampler linearSampler;


void main()
{
	const float fragmentDepth = texture(sampler2D(depth, linearSampler), uv).x;

	vec4 worldSpaceFragmentPos = camera.invertedViewProj * vec4((uv - 0.5f) * 2.0f, fragmentDepth, 1.0f);
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	vec3 normal;
    normal = texture(sampler2D(vertexNormals, linearSampler), uv).xyz;
    normal = remapNormals(normal);
    normal = normalize(normal);

    const vec3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    const vec4 baseAlbedo = texture(sampler2D(Albedo, linearSampler), uv);
    const float roughness = texture(sampler2D(MetalnessRoughness, linearSampler), uv).y;
    const float metalness = texture(sampler2D(MetalnessRoughness, linearSampler), uv).x;


	const vec3 lightDir = reflect(-viewDir, normal);
    const float NoV = dot(normal, viewDir);

	const float lodLevel = roughness * 10.0f;

	const vec3 radiance = texture(samplerCube(ConvolvedSkybox, linearSampler), lightDir, lodLevel).xyz;

    const vec3 irradiance = texture(samplerCube(skyBox, linearSampler), normal).xyz;

    const vec2 f_ab = texture(sampler2D(DFG, linearSampler), vec2(NoV, roughness)).xy;

    const vec3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const vec3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    frameBuffer = vec4(specular + diffuse, 1.0);
}