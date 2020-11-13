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
#include "UniformBuffers.hlsl"
#include "PBR.hlsl"

[[vk::binding(0)]]
ConstantBuffer<SSAOBuffer> ssaoOffsets;

[[vk::binding(1)]]
ConstantBuffer<CameraBuffer> camera;

[[vk::binding(2)]]
Texture2D<float2> CS_Z_buffer;

[[vk::binding(3)]]
SamplerState linearSampler;


// Total number of direct samples to take at each pixel
#define NUM_SAMPLES (11)

<<<<<<< Updated upstream
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
=======
// If using depth mip levels, the log of the maximum pixel offset before we need to switch to a lower 
// miplevel to maintain reasonable spatial locality in the cache
// If this number is too small (< 3), too many taps will land in the same pixel, and we'll get bad variance that manifests as flashing.
// If it is too high (> 5), we'll get bad performance because we're not using the MIP levels effectively
#define LOG_MAX_OFFSET 3

// This must be less than or equal to the MAX_MIP_LEVEL defined in SSAO.cpp
#define MAX_MIP_LEVEL 8

/** Used for preventing AO computation on the sky (at infinite depth) and defining the CS Z to bilateral depth key scaling. 
    This need not match the real far plane*/
#define FAR_PLANE_Z (300.0)

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

/** Reconstructs screen-space unit normal from screen-space position */
float3 reconstructCSFaceNormal(float3 C) {
  return normalize(cross(ddy(C), ddx(C)));
}

/** Returns a unit vector and a screen-space radius for the tap on a unit disk (the caller should scale by the actual disk radius) */
float2 tapLocation(int sampleNumber, float spinAngle, out float ssR){
  // Radius relative to ssR
  float alpha = float(sampleNumber + 0.5) * (1.0 / NUM_SAMPLES);
  float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;

  ssR = alpha;
  return float2(cos(angle), sin(angle));
}


/** Used for packing Z into the GB channels */
float CSZToKey(float z) {
  return clamp(z * (1.0 / camera.farPlane), 0.0, 1.0);
>>>>>>> Stashed changes
}

 
<<<<<<< Updated upstream
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
=======
/** Read the camera-space position of the point at screen-space pixel ssP */
float3 getPosition(int2 ssP) {
  float3 P;
>>>>>>> Stashed changes

  P.z = -CS_Z_buffer.Load(int3(ssP, 0)).r;

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
  P.z = -CS_Z_buffer.Load(int3(ssP >> mipLevel, mipLevel)).r;

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


float main(const PositionAndUVVertOutput pixel)
{

  // Pixel being shaded 
  int2 ssC = pixel.uv * camera.frameBufferSize;

  // World space point being shaded
  float3 C = getPosition(ssC);

  // Hash function used in the HPG12 AlchemyAO paper
  float randomPatternRotationAngle = (3 * ssC.x ^ ssC.y + ssC.x * ssC.y) * 10;

  // Reconstruct normals from positions. These will lead to 1-pixel black lines
  // at depth discontinuities, however the blur will wipe those out so they are not visible
  // in the final image.
  float3 n_C = reconstructCSFaceNormal(C);

  // Choose the screen-space sample radius
  // proportional to the projected area of the sphere
  float ssDiskRadius = -ssaoOffsets.projScale * ssaoOffsets.radius / C.z;

  float sum = 0.0;
  for (int i = 0; i < NUM_SAMPLES; ++i) {
       sum += sampleAO(ssC, C, n_C, ssDiskRadius, i, randomPatternRotationAngle);
  }

        float temp = (ssaoOffsets.radius * ssaoOffsets.radius) * ssaoOffsets.radius;
        sum /= temp * temp;
  float A = max(0.0, 1.0 - sum * ssaoOffsets.intensity * (5.0 / NUM_SAMPLES));

  // Bilateral box-filter over a quad for free, respecting depth edges
  // (the difference that this makes is subtle)
  if (abs(ddx(C.z)) < 0.02) {
    A -= ddx(A) * ((ssC.x & 1) - 0.5);
  }
  if (abs(ddy(C.z)) < 0.02) {
    A -= ddy(A) * ((ssC.y & 1) - 0.5);
  }

  return A;
}