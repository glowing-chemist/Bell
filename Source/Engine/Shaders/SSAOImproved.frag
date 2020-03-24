#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "Hammersley.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float> depthTexture;

[[vk::binding(3)]]
Texture2D<float3> normals;

[[vk::binding(4)]]
SamplerState linearSampler;


float main(PositionAndUVVertOutput vertInput)
{     
  const float area = 0.0075;
  const float falloff = 0.000001;
  
  const float radius = 0.002;

  const uint maxSize = 4;// 2 x 2
  const uint flattenedPosition = (uint(camera.frameBufferSize.y *  vertInput.uv.y) % 3) * (uint(camera.frameBufferSize.x *  vertInput.uv.x) % 3);
  const float3 random = Hamersley_uniform(flattenedPosition, maxSize);

  const float depth = 1.0f - depthTexture.Sample(linearSampler, vertInput.uv);
 
  const float3 position = float3( vertInput.uv, depth);
  float3 normal = normals.Sample(linearSampler, vertInput.uv);
  normal = remapNormals(normal);
  normal = normalize(normal);

  const float radius_depth = radius / depth;
  float occlusion = 0.0;
  for(int i=0; i < 16; i++) {
  
    float3 ray = radius_depth * reflect(ssaoOffsets.offsets[i].xyz, random);
    float3 hemi_ray = position + sign(dot(ray,normal)) * ray;
    
    float occ_depth = 1.0f - depthTexture.Sample(linearSampler, saturate(hemi_ray.xy));
    float difference = depth - occ_depth;
    
    occlusion += step(falloff, difference) * (1.0-smoothstep(falloff, area, difference));
  }
  
  float ao = 1.0 - occlusion * (1.0 / 16.0f);
  return saturate(ao);
}