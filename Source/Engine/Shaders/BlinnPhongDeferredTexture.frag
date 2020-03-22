#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "ClusteredLighting.glsl"
#include "UniformBuffers.glsl"
#include "NormalMapping.glsl"


#define MAX_MATERIALS 256


layout(location = 0) in float2 uv;

layout(location = 0) out float4 frameBuffer;

layout(set = 0, binding = 0) uniform LightBuffers
{
	Light light;
} lights[];

layout(binding = 1) uniform cameraBuffer
{
	CameraBuffer camera;
};


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

layout(push_constant) uniform numberOfLIghts
{
	float4x4 lightParams;
};


void main()
{
	const float fragmentDepth = texture(sampler2D(depth, linearSampler), uv).x;
	float4 worldSpaceFragmentPos = (camera.invertedCamera * float4(uv, fragmentDepth, 1.0f));
    worldSpaceFragmentPos /= worldSpaceFragmentPos.w;

	const uint materialID = texture(usampler2D(materialIDTexture, linearSampler), uv).x;

	float3 vertexNormal;
    vertexNormal = texture(sampler2D(vertexNormals, linearSampler), uv).xyz;
    vertexNormal = remapNormals(vertexNormal.xy);
    vertexNormal = normalize(vertexNormal);

	const float4 fragUVwithDifferentials = texture(sampler2D(uvWithDerivitives, linearSampler), uv);


    const float2 xDerivities = unpackUnorm2x16(floatBitsToUint(fragUVwithDifferentials.z));
    const float2 yDerivities = unpackUnorm2x16(floatBitsToUint(fragUVwithDifferentials.w));

    const float3 baseAlbedo = textureGrad(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler),
                                fragUVwithDifferentials.xy,
                                xDerivities,
                                yDerivities).xyz;

    float3 normal = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 1], linearSampler),
                                fragUVwithDifferentials.xy).xyz;

    // remap normal
    normal = remapNormals(normal);
    normal = normalize(normal);

    const float roughness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 2], linearSampler),
                                fragUVwithDifferentials.xy).x;

    const float metalness = texture(sampler2D(materials[nonuniformEXT(materialID * 4) + 3], linearSampler),
                                fragUVwithDifferentials.xy).x;
    
    // then calculate lighting as usual    
    float3 diffuseLighting = float3(0.0f);
    float3 specularLighting = float3(0.0f);
    float3 viewDir = normalize(camera.position - worldSpaceFragmentPos.xyz);

    {
        float3x3 tbv = tangentSpaceMatrix(vertexNormal, viewDir, float4(xDerivities, yDerivities));

        normal = tbv * normal;

        normal = normalize(normal);
    }

    // containes the number of lights. 
    for(int i = 0; i < lightParams[0].x; ++i)    
    {    
        // diffuse    
        float3 lightDir = lights[i].light.position.xyz  - worldSpaceFragmentPos.xyz;
        const float lightDistance = length(lightDir);
        lightDir = normalize(lightDir); 
        float3 diffuse = max(dot(normal, lightDir), 0.0) * lights[i].light.albedo.xzy;    
        diffuseLighting += diffuse / lightDistance;

        // specular
        float3 halfVector = normalize(lightDir + viewDir);
        float NdotH = dot( normal, halfVector );
        specularLighting += pow(clamp(NdotH, 0.0f, 1.0f), 16.0f) * lights[i].light.albedo.xzy / lightDistance;
    }

    const float3 reflect = reflect(-viewDir, normal); 
    const float lodLevel = roughness * 10.0f;

    const float3 radiance = texture(samplerCube(ConvolvedSkybox, linearSampler), reflect, lodLevel).xyz;

    const float3 irradiance = texture(samplerCube(skyBox, linearSampler), normal, 0).xyz;
    
    frameBuffer = float4(baseAlbedo * (specularLighting + irradiance) + (diffuseLighting + radiance), 1.0f);  
}
