#ifndef CONSTANTBUFFERS_HPP
#define CONSTANTBUFFERS_HPP

#include <array>


struct CameraBuffer
{
    float4x4 mViewMatrix;
    float4x4 mPerspectiveMatrix;
    float4x4 mViewProjMatrix;

    float4x4 mInvertedView;
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

    float4 mSceneSize;
};


struct SSAOBuffer
{
    float projScale;
    float radius;
    float bias;
    float intensity;
    float4 projInfo;
    float2 randomOffset;
};


struct Light
{
    float4 position;
    float4 albedo;
	float influence;
	float FOV;
	float brightness;
};

#endif

