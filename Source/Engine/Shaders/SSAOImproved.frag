#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable


#include "UniformBuffers.glsl"
#include "NormalMapping.glsl"
#include "Hammersley.glsl"


layout(location = 0) in vec2 uv;


layout(location = 0) out float SSAOOutput;

layout(binding = 0) uniform UniformBuffer1
{
  SSAOBuffer ssaoOffsets;
};

layout(binding = 1) uniform UniformBuffer2
{        
    CameraBuffer camera;   
}; 

layout(binding = 2) uniform texture2D depthTexture;

layout(binding = 3) uniform texture2D normals;

layout(binding = 4) uniform sampler linearSampler;


void main()
{     
  const float area = 0.0075;
  const float falloff = 0.000001;
  
  const float radius = 0.002;

  const uint maxSize = 4;// 2 x 2
  const uint flattenedPosition = (uint(camera.frameBufferSize.y * uv.y) % 3) * (uint(camera.frameBufferSize.x * uv.x) % 3);
  const vec3 random = Hamersley_uniform(flattenedPosition, maxSize);

  const float depth = texture(sampler2D(depthTexture, linearSampler), uv).x;
 
  const vec3 position = vec3(uv, depth);
  vec3 normal = texture(sampler2D(normals, linearSampler), uv).xyz;
  normal = remapNormals(normal);
  normal = normalize(normal);

  const float radius_depth = radius / depth;
  float occlusion = 0.0;
  for(int i=0; i < 16; i++) {
  
    vec3 ray = radius_depth * reflect(ssaoOffsets.offsets[i].xyz, random);
    vec3 hemi_ray = position + sign(dot(ray,normal)) * ray;
    
    float occ_depth = texture(sampler2D(depthTexture, linearSampler), clamp(hemi_ray.xy, 0.0f, 1.0f)).x;
    float difference = depth - occ_depth;
    
    occlusion += step(falloff, difference) * (1.0-smoothstep(falloff, area, difference));
  }
  
  float ao = 1.0 - occlusion * (1.0 / 16.0f);
  SSAOOutput = clamp(ao, 0.0f, 1.0f);
}