#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable


#include "UniformBuffers.glsl"


layout(location = 0) in float2 uv;


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

layout(binding = 3) uniform sampler linearSampler;


float3 normalsFromDepth(float depth, float2 texcoords) {
  
  const float2 yoffset = float2(0.0,0.001);
  const float2 xoffset = float2(0.001,0.0);
  
  float depth1 = texture(sampler2D(depthTexture, linearSampler), texcoords + yoffset).x;
  float depth2 = texture(sampler2D(depthTexture, linearSampler), texcoords + xoffset).x;
  
  float3 p1 = float3(yoffset, depth1 - depth);
  float3 p2 = float3(xoffset, depth2 - depth);
  
  float3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}

void main()
{     
  const float area = 0.0075;
  const float falloff = 0.000001;
  
  const float radius = 0.002;

  const float3 random_offsets[4] = 
  {
  	float3( 0.7119,-0.0154, 0.0352), float3(-0.0533, 0.2847,-0.5411),
    float3( -0.0918,-0.0631, 0.5460), float3(-0.4776, 0.0596,-0.0271)
  };
  
  int xoffset = int(uv.x * camera.frameBufferSize.x) % 2;
  int yoffset = int(uv.y * camera.frameBufferSize.y) % 2;
  const float3 random = normalize( random_offsets[xoffset + yoffset] );
  
  const float depth = 1.0f - texture(sampler2D(depthTexture, linearSampler), uv).x;
 
  const float3 position = float3(uv, depth);
  const float3 normal = normalsFromDepth(depth, uv);
  
  const float radius_depth = radius / depth;
  float occlusion = 0.0;
  for(int i=0; i < 16; i++) {
  
    float3 ray = radius_depth * reflect(ssaoOffsets.offsets[i].xyz, random);
    float3 hemi_ray = position + sign(dot(ray,normal)) * ray;
    
    float occ_depth = 1.0f - texture(sampler2D(depthTexture, linearSampler), clamp(hemi_ray.xy, 0.0f, 1.0f)).x;
    float difference = depth - occ_depth;
    
    occlusion += step(falloff, difference) * (1.0-smoothstep(falloff, area, difference));
  }
  
  float ao = 1.0 - occlusion * (1.0 / 16.0f);
  SSAOOutput = clamp(ao, 0.0f, 1.0f);
}
