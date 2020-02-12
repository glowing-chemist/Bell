#ifndef CONSTANTBUFFERS_HPP
#define CONSTANTBUFFERS_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>


struct CameraBuffer
{
    glm::mat4 mViewMatrix;
    glm::mat4 mPerspectiveMatrix;
    glm::mat4 mViewProjMatrix;

    glm::mat4 mInvertedViewProjMatrix;
	glm::mat4 mInvertedPerspective;

    glm::mat4 mPreviousFrameViewProjMatrix;

    float mNeaPlane;
    float mFarPlane;
	
    glm::vec2 mFrameBufferSize;

    glm::vec3 mPosition;

    float mFOV;

    glm::vec2 mJitter;
    glm::vec2 mPreviousJitter;
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

