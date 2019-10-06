#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "UniformBuffers.glsl"
#include "NormalMapping.glsl"


layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 frameBuffer;

layout(binding = 0) uniform LightBuffers
{
	Light light;
} lights[];

layout(binding = 1) uniform cameraBuffer
{
	CameraBuffer camera;
};

layout(binding = 2) uniform materiaIndex
{
	int mappedIndex;
} materialIndexMapping[];

layout(binding = 3) uniform texture2D depth;
layout(binding = 4) uniform texture2D vertexNormals;
layout(binding = 5) uniform utexture2D materialIDTexture;
layout(binding = 6) uniform texture2D uvWithDerivitives;
layout(binding = 7) uniform sampler linearSampler;

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
layout(binding = 8) uniform texture2D materials[];

layout(push_constant) uniform numberOfLIghts
{
	mat4 lightParams;
};


void main()
{
	const float fragmentDepth = texture(sampler2D(depth, linearSampler), uv).x;
	const vec3 worldSpaceFragmentPos = (camera.invertedCamera * vec4(uv, fragmentDepth, 1.0f)).xyz;

	const uint materialID = texture(usampler2D(materialIDTexture, linearSampler), uv).x;

	const vec3 vertexNormal = (texture(sampler2D(vertexNormals, linearSampler), uv).xyz * 2.0f) - 1.0f;

	const vec4 fragUVwithDifferentials = texture(sampler2D(uvWithDerivitives, linearSampler), uv);


    const vec2 xDerivities = unpackUnorm2x16(floatBitsToUint(fragUVwithDifferentials.z));
    const vec2 yDerivities = unpackUnorm2x16(floatBitsToUint(fragUVwithDifferentials.w));

    vec3 albedo = textureGrad(sampler2D(materials[nonuniformEXT(materialIndexMapping[materialID].mappedIndex)], linearSampler),
    							fragUVwithDifferentials.xy, // UV
    							xDerivities,
    							yDerivities).xyz;

    vec3 normal = texture(sampler2D(materials[nonuniformEXT(materialIndexMapping[materialID].mappedIndex) + 1], linearSampler),
                                fragUVwithDifferentials.xy).xyz;

    normal = remapNormals(normal);
    
    // then calculate lighting as usual    
    vec3 lighting = albedo * 0.1; // hard-coded ambient component. TODO maybe replace this with image based lighting.  
    vec3 viewDir = normalize(camera.position - worldSpaceFragmentPos);

    {
        mat3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, vec4(xDerivities, yDerivities));

        normal = tbv * normal;

        normal = normalize(normal);
    }

    // containes the number of lights. 
    for(int i = 0; i < lightParams[0].x; ++i)    
    {    
        // diffuse    
        vec3 lightDir = normalize(lights[i].light.position.xyz  - worldSpaceFragmentPos);    
        vec3 diffuse = max(dot(normal, lightDir), 0.0) * albedo * lights[i].light.albedo.xzy;    
        lighting += diffuse;    
    }    
    
    frameBuffer = vec4(albedo, 1.0) + vec4(lighting, 1.0);  
}
