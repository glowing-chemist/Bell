#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "Hammersley.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float> linearisedDepth;

[[vk::binding(3)]]
Texture2D<float3> normals;

[[vk::binding(4)]]
SamplerState linearSampler;


float main(PositionAndUVVertOutput vertInput)
{     
  const float radius = 0.0002;

  const uint maxSize = 4;// 2 x 2
  const uint flattenedPosition = (uint(camera.frameBufferSize.y *  vertInput.uv.y) % 3) * (uint(camera.frameBufferSize.x *  vertInput.uv.x) % 3);
  const float3 random = Hamersley_uniform(flattenedPosition, maxSize);

  const float depth = linearisedDepth.SampleLevel(linearSampler, vertInput.uv, 0.0f);
  if(depth == 1.0f)
    return depth;
 
  const float3 position = float3( vertInput.uv, depth);
  float3 normal = normals.Sample(linearSampler, vertInput.uv);
  normal = remapNormals(normal);
  normal = normalize(normal);
  // bring in to view space
  normal = normalize(mul((float3x3)camera.view, normal));

  float occlusion = 0.0;
  const float radius_depth = radius / depth;
  for(int i = 0; i < 16; i++) {
  
    float3 ray = radius_depth * reflect(ssaoOffsets.offsets[i].xyz, random);
    float3 hemi_ray = position + sign(dot(normalize(ray),normal)) * ray;
    
    float occ_depth = linearisedDepth.SampleLevel(linearSampler, saturate(hemi_ray.xy), 0.0f);
    
    occlusion += float(hemi_ray.z < occ_depth);
  }
  
  float ao = occlusion * (1.0f / 16.0f);
  return saturate(ao);
}