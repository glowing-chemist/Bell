#ifndef CONSTANTBUFFERS_HPP
#define CONSTANTBUFFERS_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>


struct CameraBuffer
{
    float4x4 mViewMatrix;
    float4x4 mPerspectiveMatrix;
    float4x4 mViewProjMatrix;

    float4x4 mInvertedViewProjMatrix;
    float4x4 mInvertedPerspective;

    float4x4 mPreviousFrameViewProjMatrix;

    float mNeaPlane;
    float mFarPlane;
	
    float2 mFrameBufferSize;

    float3 mPosition;

    float mFOV;

    float2 mJitter;
    float2 mPreviousJitter;
};


struct SSAOBuffer
{
	glm::vec4 mOffsets[64];
    float mScale;
    int mOffsetsCount;
};


struct Light
{
	glm::vec4 position;
	glm::vec4 albedo;
	float influence;
	float FOV;
	float brightness;
};

#endif

