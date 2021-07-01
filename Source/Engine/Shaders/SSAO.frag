/**
 \file SAO_AO.pix
 \author Morgan McGuire and Michael Mara, NVIDIA Research

 Reference implementation of the Scalable Ambient Obscurance (SAO) screen-space ambient obscurance algorithm. 
 
 The optimized algorithmic structure of SAO was published in McGuire, Mara, and Luebke, Scalable Ambient Obscurance,
 <i>HPG</i> 2012, and was developed at NVIDIA with support from Louis Bavoil.

 The mathematical ideas of AlchemyAO were first described in McGuire, Osman, Bukowski, and Hennessy, The 
 Alchemy Screen-Space Ambient Obscurance Algorithm, <i>HPG</i> 2011 and were developed at 
 Vicarious Visions.  
 
 DX11 HLSL port by Leonardo Zide of Treyarch

 <hr>

  Open Source under the "BSD" license: http://www.opensource.org/licenses/bsd-license.php

  Copyright (c) 2011-2012, NVIDIA
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  */

  /*
  Ported to BELL 13/11/2020.
  */

#include "VertexOutputs.hlsl"
#include "NormalMapping.hlsl"
#include "UniformBuffers.hlsl"
#include "Utilities.hlsl"
#include "PBR.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float2> CS_Z_buffer;

[[vk::binding(3)]]
Texture2D<float2> prevLinearDepth;

[[vk::binding(4)]]
Texture2D<float> depth;

[[vk::binding(5)]]
Texture2D<float> history;

[[vk::binding(6)]]
RWTexture2D<uint> historyCounter;

[[vk::binding(7)]]
Texture2D<uint> prevHistoryCounter;

[[vk::binding(8)]]
Texture2D<float2> velocity;

[[vk::binding(9)]]
Texture2D<float2> normals;

[[vk::binding(10)]]
SamplerState linearSampler;


// Total number of direct samples to take at each pixel
#define NUM_SAMPLES (9)

// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower 
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET 4

// This must be less than or equal to the MAX_MIP_LEVEL defined in SSAO.cpp
#define MAX_MIP_LEVEL 8

// This is the number of turns around the circle that the spiral pattern makes.  This should be prime to prevent
// taps from lining up.  This particular choice was tuned for NUM_SAMPLES == 9
#define NUM_SPIRAL_TURNS (7)

//////////////////////////////////////////////////
/*
/** The height in pixels of a 1m object if viewed from 1m away.  
    You can compute it from your projection matrix.  The actual value is just
    a scale factor on radius; you can simply hardcode this to a constant (~500)
    and make your radius value unitless (...but resolution dependent.)  
float projScale;

/** World-space AO radius in scene units (r).  e.g., 1.0m 
float radius;

/** Bias to avoid AO in smooth corners, e.g., 0.01m 
float bias;

/** Darkending factor, e.g., 1.0 
float intensity;

/**  vec4(-2.0f / (width*P[0][0]), 
          -2.0f / (height*P[1][1]),/
          ( 1.0f - P[0][2]) / P[0][0], 
          ( 1.0f + P[1][2]) / P[1][1])
    
    where P is the projection matrix that maps camera space points 
    to [-1, 1] x [-1, 1].  That is, GCamera::getProjectUnit(). 
float4 projInfo;

float radius2 = radius * radius;
*/
/////////////////////////////////////////////////////////

/** Reconstruct camera-space P.xyz from screen-space S = (x, y) in
    pixels and camera-space z < 0.  Assumes that the upper-left pixel center
    is at (0.5, 0.5) [but that need not be the location at which the sample tap 
    was placed!]

    Costs 3 MADD.  Error is on the order of 10^3 at the far plane, partly due to z precision.
  */
float3 reconstructCSPosition(float2 S, float z) {
  return float3((S.xy * ssaoOffsets.projInfo.xy + ssaoOffsets.projInfo.zw) * z, z);
}

float getViewDepth(const int3 pos)
{
    return  CS_Z_buffer.Load(pos).x;
}

/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
float2 tapLocation(int sampleNumber, float spinAngle, out float ssR){
  // Radius relative to ssR
  float alpha = float(sampleNumber + 0.5) * (1.0 / NUM_SAMPLES);
  float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;

  ssR = alpha;
  return float2(cos(angle), sin(angle));
}


/** Read the camera-space position of the point at screen-space pixel ssP */
float3 getPosition(int2 ssP) {
  float3 P;

  P.z = getViewDepth(int3(ssP, 0));

  // Offset to pixel center
  P = reconstructCSPosition(float2(ssP) + float2(0.5, 0.5), P.z);
  return P;
}


/** Read the camera-space position of the point at screen-space pixel ssP + unitOffset * ssR.  Assumes length(unitOffset) == 1 */
float3 getOffsetPosition(int2 ssC, float2 unitOffset, float ssR) {
  // Derivation:
  //  mipLevel = floor(log(ssR / MAX_OFFSET));
  int mipLevel = clamp((int)floor(log2(ssR)) - LOG_MAX_OFFSET, 0, MAX_MIP_LEVEL);

  int2 ssP = int2(ssR*unitOffset) + ssC;

  float3 P;

  // Divide coordinate by 2^mipLevel
  P.z = getViewDepth(int3(ssP >> mipLevel, mipLevel));

  // Offset to pixel center
  P = reconstructCSPosition(float2(ssP) + float2(0.5, 0.5), P.z);

  return P;
}


/** Compute the occlusion due to sample with index \a i about the pixel at \a ssC that corresponds
    to camera-space point \a C with unit normal \a n_C, using maximum screen-space sampling radius \a ssDiskRadius */
float sampleAO(in int2 ssC, in float3 C, in float3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle) {
  // Offset on the unit disk, spun for this pixel
  float ssR;
  float2 unitOffset = tapLocation(tapIndex, randomPatternRotationAngle, ssR);
  ssR *= ssDiskRadius;

  // The occluding point in camera space
  float3 Q = getOffsetPosition(ssC, unitOffset, ssR);

  float3 v = Q - C;

  float vv = dot(v, v);
  float vn = dot(v, n_C);

    const float epsilon = 0.01;
    float f = max((ssaoOffsets.radius * ssaoOffsets.radius) - vv, 0.0); return f * f * f * max((vn - ssaoOffsets.bias) / (epsilon + vv), 0.0);
}

float ssao(const uint2 pixel)
{
  // World space point being shaded
  float3 C = getPosition(pixel);

  // Hash function used in the HPG12 AlchemyAO paper
  const uint2 randomOffset = ssaoOffsets.randomOffset * camera.frameBufferSize;
  const uint2 randomPosition = pixel + randomOffset;
  float randomPatternRotationAngle = (3 * randomPosition.x ^ randomPosition.y + randomPosition.x * randomPosition.y) * 10;

  // Reconstruct normals from positions. These will lead to 1-pixel black lines
  // at depth discontinuities, however the blur will wipe those out so they are not visible
  // in the final image.
  float3 n_C = float3(normals.Load(int3(pixel, 0)), 0);
  n_C = decodeOct(n_C.xy);
  n_C = mul((float3x3)camera.view, n_C);
  n_C = -normalize(n_C);

  // Choose the screen-space sample radius
  // proportional to the projected area of the sphere
  float ssDiskRadius = -ssaoOffsets.projScale * ssaoOffsets.radius / C.z;

  float sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; ++i) {
       sum += sampleAO(pixel, C, n_C, ssDiskRadius, i, randomPatternRotationAngle);
  }

  float temp = (ssaoOffsets.radius * ssaoOffsets.radius) * ssaoOffsets.radius;
  sum /= temp * temp;
  float A = max(0.0, 1.0 - sum * ssaoOffsets.intensity * (5.0 / NUM_SAMPLES));

  // Bilateral box-filter over a quad for free, respecting depth edges
  // (the difference that this makes is subtle)
  if (abs(ddx(C.z)) < 0.02) {
    A -= ddx(A) * ((pixel.x & 1) - 0.5);
  }
  if (abs(ddy(C.z)) < 0.02) {
    A -= ddy(A) * ((pixel.y & 1) - 0.5);
  }

  return A;
}

float main(const PositionAndUVVertOutput pixel) : SV_Target0
{
  const float currentDepth = depth.SampleLevel(linearSampler, pixel.uv, 0.0f);
  if(currentDepth == 0.0f)
  {
      historyCounter[pixel.uv * (camera.frameBufferSize / 2)] = 1;
      return 1.0f;
  }

  // Pixel being shaded 
  int2 ssC = pixel.uv * camera.frameBufferSize;

  float ao = ssao(ssC);
  uint historyCount = 1;

  float2 pixelVelocity = velocity.SampleLevel(linearSampler, pixel.uv, 0.0f);
  pixelVelocity *= 0.5f;
  const float2 previousUV = pixel.uv - pixelVelocity;

  const float prevDepth = prevLinearDepth.SampleLevel(linearSampler, previousUV, 0.0f).x;

  float4 worldSpaceFragmentPos = mul(camera.invertedViewProj, float4((pixel.uv - 0.5f) * 2.0f, currentDepth, 1.0f));
  worldSpaceFragmentPos /= worldSpaceFragmentPos.w;
  float4 reprojectedPositionVS = mul(camera.previousView, worldSpaceFragmentPos);
  const float depthVS = -reprojectedPositionVS.z;

  const float depthDiff = abs(depthVS - prevDepth);
  
  const bool offScreen = previousUV.x > 1.0f || previousUV.x < 0.0f || previousUV.y > 1.0f || previousUV.y < 0.0f;
  
  if((depthDiff < 0.1) && !offScreen)
  {
    uint3 prevPixel = uint3(previousUV * (camera.frameBufferSize / 2), 0);
    historyCount = prevHistoryCounter.Load(prevPixel);
    const float prevAo = history.SampleLevel(linearSampler, previousUV, 0.0f);
    ao = prevAo + ((ao - prevAo) * (1.0f / historyCount));

    historyCount = min(historyCount + 1, 64);
  }

  historyCounter[pixel.uv * (camera.frameBufferSize / 2)] = historyCount;
  return ao;
}