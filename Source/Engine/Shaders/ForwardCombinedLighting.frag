#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "MeshAttributes.glsl"
#include "NormalMapping.glsl"
#include "UniformBuffers.glsl"
#include "ClusteredLighting.glsl"


layout(location = 0) in float4 positionWS;
layout(location = 1) in float3 vertexNormal;
layout(location = 2) in flat uint materialID;
layout(location = 3) in float2 uv;
layout(location = 4) in float2 inVelocity;

layout(location = 0) out float4 frameBuffer;
layout(location = 1) out float2 velocity;

layout(set = 0, binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
}; 
layout(set = 0, binding = 1) uniform texture2D DFG;
layout(set = 0, binding = 2) uniform texture2D ltcMat;
layout(set = 0, binding = 3) uniform texture2D ltcAmp;
layout(set = 0, binding = 4) uniform utexture2D activeFroxels;
layout(set = 0, binding = 5) uniform textureCube skyBox;
layout(set = 0, binding = 6) uniform textureCube ConvolvedSkybox;
layout(set = 0, binding = 7) uniform sampler linearSampler;
layout(set = 0, binding = 8) uniform sampler pointSampler;

// Light lists
layout(std430, set = 0, binding = 9) buffer readonly sparseFroxels
{
    uint2 sparseFroxelList[];
};

layout(std430, set = 0,  binding = 10) buffer readonly froxelIndicies
{
    uint indicies[];
};

#ifdef Shadow_Map
layout(set = 0, binding = 11) uniform texture2D shadowMap;
#endif

// an unbound array of matyerial parameter textures
// In order albedo, normals, rougness, metalness
layout(set = 1, binding = 0) uniform texture2D materials[];

layout(std430, set = 2, binding = 0) buffer readonly sceneLights
{
    uint4  lightCount;
    Light lights[];
};


void main()
{
	const float4 baseAlbedo = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler),
                                		uv);

    float3 normal = texture(sampler2D(materials[nonuniformEXT((materialID * 4) + 1)], linearSampler),
                                uv).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = texture(sampler2D(materials[nonuniformEXT((materialID * 4) + 2)], linearSampler),
                                uv).x;

    const float metalness = texture(sampler2D(materials[nonuniformEXT((materialID * 4) + 3)], linearSampler),
                                uv).x;

	const float3 viewDir = normalize(camera.position - positionWS.xyz);

	const float2 xDerivities = dFdxFine(uv);
	const float2 yDerivities = dFdyFine(uv);

	{
    	float3x3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, float4(xDerivities, yDerivities));

    	normal = tbv * normal;

    	normal = normalize(normal);
	}

	const float3 lightDir = reflect(-viewDir, normal);

	const float lodLevel = roughness * 10.0f;

    // Calculate contribution from enviroment.
	float3 radiance = textureLod(samplerCube(ConvolvedSkybox, linearSampler), lightDir, lodLevel).xyz;

    float3 irradiance = texture(samplerCube(skyBox, linearSampler), normal).xyz;

#ifdef Shadow_Map
    const float occlusion = texture(sampler2D(shadowMap, linearSampler), gl_FragCoord.xy / camera.frameBufferSize).x;
    radiance *= occlusion;
    irradiance *= occlusion;
#endif

	const float NoV = dot(normal, viewDir);
    float3x3 minV;
    float LTCAmp;
    float2 f_ab;
    initializeLightState(minV, LTCAmp, f_ab, DFG, ltcMat, ltcAmp, linearSampler, NoV, roughness);

    const float3 diffuse = calculateDiffuse(baseAlbedo.xyz, metalness, irradiance);
    const float3 specular = calculateSpecular(roughness * roughness, normal, viewDir, metalness, baseAlbedo.xyz, radiance, f_ab);

    float4 lighting = float4(diffuse + specular, 1.0f);

    // Calculate contribution from lights.
    const uint froxelIndex = texture(usampler2D(activeFroxels, pointSampler), gl_FragCoord.xy / camera.frameBufferSize).x;
    const uint2 lightListIndicies = sparseFroxelList[froxelIndex];

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
                lighting +=  pointLightContribution(light, positionWS, viewDir, normal, metalness, roughness, baseAlbedo.xyz, f_ab);
                break;
            }

            case 2: // Area.
            {
                lighting +=  areaLightContribution(light, positionWS, viewDir, normal, metalness, roughness, baseAlbedo.xyz, minV, LTCAmp);
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