#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "UniformBuffers.glsl"
#include "NormalMapping.glsl"


layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 vertexNormal;
layout(location = 3) in flat uint materialID;


layout(location = 0) out vec3 outAlbedo;
layout(location = 1) out vec3 outNormals;
layout(location = 2) out vec2 outMetalnessRoughness;


layout(binding = 0) uniform UniformBufferObject 
{    
    CameraBuffer camera;
}; 

layout(set = 0, binding = 1) uniform sampler linearSampler;

layout(set = 1, binding = 0) uniform texture2D materials[];


void main()
{
	const vec3 baseAlbedo = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler), uv).xyz;

    vec3 normal = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 1], linearSampler), uv).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 2], linearSampler), uv).x;

    const float metalness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 3], linearSampler), uv).x;

    const vec2 xDerivities = dFdxFine(uv);
    const vec2 yDerivities = dFdxFine(uv);

    const vec3 viewDir = normalize(camera.position - position.xyz);

	{
    	mat3 tbv = tangentSpaceMatrix(vertexNormal.xyz, viewDir, vec4(xDerivities, yDerivities));

    	normal = tbv * normal;

    	normal = normalize(normal);
    }

	outAlbedo = baseAlbedo;
	outNormals = (normal + 1.0f) * 0.5f;
	outMetalnessRoughness = vec2(metalness, roughness);
}