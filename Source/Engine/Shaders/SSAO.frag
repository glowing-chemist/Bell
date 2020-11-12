#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "PBR.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float> LinearDepth;

[[vk::binding(3)]]
SamplerState linearSampler;

#include "RayMarching.hlsl"

//#define RAY_MARCHING

float3 normalsFromDepth(float depth, float2 texcoords)
{
  
  const float2 yoffset = float2(0.0,0.001);
  const float2 xoffset = float2(0.001,0.0);
  
  float depth1 = LinearDepth.SampleLevel(linearSampler, texcoords + yoffset, 0.0f);
  float depth2 = LinearDepth.SampleLevel(linearSampler, texcoords + xoffset, 0.0f);
  
  float3 p1 = float3(yoffset, depth1 - depth);
  float3 p2 = float3(xoffset, depth2 - depth);
  
  float3 normal = cross(p1, p2);
  
  return normalize(normal);
}

float main(PositionAndUVVertOutput vertInput)
{     
  const float depth = LinearDepth.SampleLevel(linearSampler, vertInput.uv, 0.0f);
  if(depth == 1.0f)
    return depth;
 
  float3 normal = normalsFromDepth(depth, vertInput.uv);

#ifdef RAY_MARCHING

  uint width, height, mips;
  LinearDepth.GetDimensions(4, width, height, mips);

  const float3 position = float3( (vertInput.uv - 0.5f) * 2.0, depth);
  float occlusion = 1.0;
  for(int i = 0; i < 16; i++) 
  {
    float2 Xi = Hammersley(i, 16);
    float3 L;
    float  NdotL;
    importanceSampleCosDir(Xi, normal, L, NdotL);

    if(NdotL >= 0.0f)
    {
        const float2 intersectionUV = marchRay(position, L, 7, float2(1.0f, 1.0f) / float2(width, height), 3, camera.nearPlane, camera.farPlane, camera.invertedPerspective);
        if(all(intersectionUV >= float2(0.0f, 0.0f))) // valid intersection.
        {
            const float intersectionDepth = LinearDepth.SampleLevel(linearSampler, intersectionUV, 0.0f);
            const float3 intersectionPosition = float3((intersectionUV - 0.5f) * 2.0f, -intersectionDepth);

            const float intersectionDistance = length(intersectionPosition - position);

            occlusion -= (1.0f - (intersectionDistance / sqrt(2.0f))) * (1.0f / 16.0f);
        }
    }
  }

#else

  const float3 position = float3( vertInput.uv, depth);
  const float radius = 0.0002f;

  const uint maxSize = 4;// 2 x 2
  const uint flattenedPosition = (uint(camera.frameBufferSize.y *  vertInput.uv.y) % 3) * (uint(camera.frameBufferSize.x *  vertInput.uv.x) % 3);
  const float3 random = Hamersley_uniform(flattenedPosition, maxSize);

  float occlusion = 0.0;
  const float radius_depth = radius / depth;
  for(int i = 0; i < 16; i++)
  {
    float3 ray = radius_depth * reflect(ssaoOffsets.offsets[i].xyz, random);
    float3 hemi_ray = position + sign(dot(normalize(ray),normal)) * ray;
    
    float occ_depth = LinearDepth.SampleLevel(linearSampler, saturate(hemi_ray.xy), 0.0f);
    
    occlusion += uint(hemi_ray.z < occ_depth);
  }
  
  occlusion *= 1.0f / 16.0f;

#endif

  return saturate(occlusion);
}
