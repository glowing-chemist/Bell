#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "PBR.glsl"
#include "NormalMapping.glsl"
#include "UniformBuffers.glsl"


layout(location = 0) in vec4 positionWS;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in flat uint materialID;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec4 frameBuffer;

layout(set = 0, binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
}; 
layout(set = 0, binding = 1) uniform texture2D DFG;
layout(set = 0, binding = 2) uniform textureCube skyBox;
layout(set = 0, binding = 3) uniform textureCube ConvolvedSkybox;
layout(set = 0, binding = 4) uniform sampler linearSampler;

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
layout(set = 1, binding = 0) uniform texture2D materials[];

void main()
{
	const vec3 baseAlbedo = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler),
                                		uv).xyz;

    vec3 normal = texture(sampler2D(materials[nonuniformEXT((materialID * 4) + 1)], linearSampler),
                                uv).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = texture(sampler2D(materials[nonuniformEXT((materialID * 4) + 2)], linearSampler),
                                uv).x;

    const float metalness = texture(sampler2D(materials[nonuniformEXT((materialID * 4) + 3)], linearSampler),
                                uv).x;

	const vec3 viewDir = normalize(camera.position - positionWS.xyz);

	const vec2 xDerivities = dFdxFine(uv);
	const vec2 yDerivities = dFdyFine(uv);

	{
    	mat3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, vec4(xDerivities, yDerivities));

    	normal = tbv * normal;

    	normal = normalize(normal);
	}

	const vec3 lightDir = reflect(-viewDir, normal);

	const float lodLevel = roughness * 10.0f;

	const vec3 radiance = textureLod(samplerCube(ConvolvedSkybox, linearSampler), lightDir, lodLevel).xyz;

    const vec3 irradiance = texture(samplerCube(skyBox, linearSampler), normal).xyz;

	const float NoV = dot(normal, viewDir);
    const vec2 f_ab = texture(sampler2D(DFG, linearSampler), vec2(NoV, roughness)).xy;

    const vec3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const vec3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    frameBuffer = vec4(specular + diffuse, 1.0);
}