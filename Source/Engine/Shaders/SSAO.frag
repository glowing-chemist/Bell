#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float> linearisedDepth;

[[vk::binding(3)]]
SamplerState linearSampler;


float3 normalsFromDepth(float depth, float2 texcoords) {
  
  const float2 yoffset = float2(0.0,0.001);
  const float2 xoffset = float2(0.001,0.0);
  
  float depth1 = linearisedDepth.Sample(linearSampler, texcoords + yoffset);
  float depth2 = linearisedDepth.Sample(linearSampler, texcoords + xoffset);
  
  float3 p1 = float3(yoffset, depth1 - depth);
  float3 p2 = float3(xoffset, depth2 - depth);
  
  float3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}

float main(PositionAndUVVertOutput vertInput)
{     
  const float area = 0.0075;
  const float falloff = 0.000001;
  const float radius = 0.00002;

  const float3 random_offsets[4] = 
  {
  	float3( 0.7119,-0.0154, 0.0352), float3(-0.0533, 0.2847,-0.5411),
    float3( -0.0918,-0.0631, 0.5460), float3(-0.4776, 0.0596,-0.0271)
  };
  
  int xoffset = int(vertInput.uv.x * camera.frameBufferSize.x) % 2;
  int yoffset = int(vertInput.uv.y * camera.frameBufferSize.y) % 2;
  const float3 random = normalize( random_offsets[xoffset + yoffset] );
  
  const float depth = linearisedDepth.Sample(linearSampler, vertInput.uv);
 
  const float3 position = float3(vertInput.uv, depth);
  const float3 normal = normalsFromDepth(depth, vertInput.uv);
  
  float occlusion = 0.0;
  for(int i = 0; i < 16; i++) {
  
    float3 ray = radius * reflect(ssaoOffsets.offsets[i].xyz, random);
    float3 hemi_ray = position + sign(dot(ray,normal)) * ray;
    
    float occ_depth = linearisedDepth.Sample(linearSampler, saturate(hemi_ray.xy));
    float difference = hemi_ray.z - occ_depth;
    
    occlusion += step(falloff, difference) * (1.0f - smoothstep(falloff, area, difference));
  }
  
  float ao = 1.0f - (occlusion * (1.0f / 16.0f));
  return saturate(ao);
}
