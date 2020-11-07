#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"
#include "NormalMapping.hlsl"
#include "PBR.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float> LinearDepth;

[[vk::binding(3)]]
Texture2D<float2> normals;

[[vk::binding(4)]]
SamplerState linearSampler;


#include "RayMarching.hlsl"

//#define RAY_MARCHING


float main(PositionAndUVVertOutput vertInput)
{     
  const float depth = LinearDepth.SampleLevel(linearSampler, vertInput.uv, 0.0f);
  if(depth == 1.0f)
    return depth;
 
  const uint2 pixel = vertInput.uv * camera.frameBufferSize;
  float3 normal = decodeOct(normals.Load(uint3(pixel, 0)));
  normal = normalize(normal);
  // bring in to view space
  normal = normalize(mul((float3x3)camera.view, normal));
  normal.z *= -1.0f; // Put positive z facing away from the camera.

#ifdef RAY_MARCHING

  const float3 position = float3( (vertInput.uv - 0.5f) * 2.0f, depth);

  uint width, height, mips;
  LinearDepth.GetDimensions(4, width, height, mips);

  float occlusion = 1.0;
  for(int i = 0; i < 16; i++) 
  {
    float2 Xi = Hammersley(i, 16);
    float3 L;
    float  NdotL;
    importanceSampleCosDir(Xi, normal, L, NdotL);

    if(NdotL >= 0.0f)
    {
        const float2 intersectionUV = marchRay(position, L, 5, float2(1.0f, 1.0f) / float2(width, height));
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