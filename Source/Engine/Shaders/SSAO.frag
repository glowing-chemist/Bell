#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "Hammersley.hlsl"

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
  const float radius = 0.0002;

  const uint maxSize = 4;// 2 x 2
  const uint flattenedPosition = (uint(camera.frameBufferSize.y *  vertInput.uv.y) % 3) * (uint(camera.frameBufferSize.x *  vertInput.uv.x) % 3);
  const float3 random = Hamersley_uniform(flattenedPosition, maxSize);

  const float depth = linearisedDepth.Sample(linearSampler, vertInput.uv);
 
  const float3 position = float3( vertInput.uv, depth);
  const float3 normal = normalsFromDepth(depth, vertInput.uv);
  // bring in to view space
  normal = normalize(mul((float3x3)camera.view, normal));

  float occlusion = 0.0;
  for(int i = 0; i < 16; i++) {
  
    float3 ray = radius * reflect(ssaoOffsets.offsets[i].xyz, random);
    float3 hemi_ray = position + sign(dot(ray,normal)) * ray;
    
    float occ_depth = linearisedDepth.Sample(linearSampler, saturate(hemi_ray.xy));
    float difference = hemi_ray.z - occ_depth;
    
    occlusion += uint(hemi_ray.z < occ_depth);
  }
  
  float ao = 1.0f - (occlusion * (1.0f / 16.0f));
  return saturate(ao);
}
