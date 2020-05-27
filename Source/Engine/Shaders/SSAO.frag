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

float3 normalsFromDepth(float depth, float2 texcoords) {
  
  const float2 yoffset = float2(0.0,0.001);
  const float2 xoffset = float2(0.001,0.0);
  
  float depth1 = LinearDepth.Sample(linearSampler, texcoords + yoffset);
  float depth2 = LinearDepth.Sample(linearSampler, texcoords + xoffset);
  
  float3 p1 = float3(yoffset, depth1 - depth);
  float3 p2 = float3(xoffset, depth2 - depth);
  
  float3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}

float main(PositionAndUVVertOutput vertInput)
{     
  const float depth = LinearDepth.SampleLevel(linearSampler, vertInput.uv, 0.0f);
  if(depth == 1.0f)
    return depth;
 
  const float3 position = float3( (vertInput.uv - 0.5f) * 2.0, depth);
  float3 normal = normalsFromDepth(depth, vertInput.uv);
  // bring in to view space
  normal = normalize(mul((float3x3)camera.view, normal));
  normal.z *= -1.0f; // Put positive z facing away from the camera.

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
  
  return saturate(occlusion);
}
