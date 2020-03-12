#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "MeshAttributes.glsl"
#include "NormalMapping.glsl"
#include "UniformBuffers.glsl"
#include "ClusteredLighting.glsl"


layout(location = 0) in vec4 positionWS;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in flat uint materialID;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec2 inVelocity;

layout(location = 0) out vec4 frameBuffer;
layout(location = 1) out vec2 velocity;

layout(set = 0, binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
}; 
layout(set = 0, binding = 1) uniform texture2D DFG;
layout(set = 0, binding = 2) uniform utexture2D activeFroxels;
layout(set = 0, binding = 3) uniform textureCube skyBox;
layout(set = 0, binding = 4) uniform textureCube ConvolvedSkybox;
layout(set = 0, binding = 5) uniform sampler linearSampler;
layout(set = 0, binding = 6) uniform sampler pointSampler;

// Light lists
layout(std430, set = 0, binding = 7) buffer readonly sparseFroxels
{
    uvec2 sparseFroxelList[];
};

layout(std430, set = 0,  binding = 8) buffer readonly froxelIndicies
{
    uint indicies[];
};

#ifdef Shadow_Map
layout(set = 0, binding = 9) uniform texture2D shadowMap;
#endif

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
layout(set = 1, binding = 0) uniform texture2D materials[];

layout(std430, set = 2, binding = 0) buffer readonly sceneLights
{
    uvec4  lightCount;
    Light lights[];
};


void main()
{
	const vec4 baseAlbedo = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler),
                                		uv);

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

    // Calculate contribution from enviroment.
	vec3 radiance = textureLod(samplerCube(ConvolvedSkybox, linearSampler), lightDir, lodLevel).xyz;

    vec3 irradiance = texture(samplerCube(skyBox, linearSampler), normal).xyz;

#ifdef Shadow_Map
    const float occlusion = texture(sampler2D(shadowMap, linearSampler), gl_FragCoord.xy / camera.frameBufferSize).x;
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(normal, viewDir);
    const vec2 f_ab = texture(sampler2D(DFG, linearSampler), vec2(NoV, roughness)).xy;

    const vec3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const vec3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    vec4 lighting = vec4(diffuse + specular, 1.0f);

    // Calculate contribution from lights.
    const uint froxelIndex = texture(usampler2D(activeFroxels, pointSampler), gl_FragCoord.xy / camera.frameBufferSize).x;
    const uvec2 lightListIndicies = sparseFroxelList[froxelIndex];

    for(uint i = 0; i < lightListIndicies.y; ++i)
    {
        uint indirectLightIndex = indicies[lightListIndicies.x + i];
        const Light light = lights[indirectLightIndex];

        switch(uint(light.type))
        {
            case 0:
            {
                lighting += pointLightContribution(light, positionWS, viewDir, normal, metalness, roughness, baseAlbedo.xyz, f_ab);
                break;
            }

            case 1: // Spot.
            {
                vec4 lighting =  pointLightContribution(light, worldSpaceFragmentPos, viewDir, normal, metalness, roughness, baseAlbedo, DFG, linearSampler);
                accum += lighting;  
                break;
            }

            case 2: // Area.
            {
                vec4 lighting =  areaLightContribution(light, worldSpaceFragmentPos, viewDir, normal, metalness, roughness, baseAlbedo, ltcMat, ltcAmp, linearSampler);
                accum += lighting;  
                break;
            }

            // Should never hit here.
            default:
                continue;
        }
    }

    if(baseAlbedo.w == 0.0f)
        discard;

    frameBuffer = lighting;
    velocity = (inVelocity * 0.5f) + 0.5f;
}