#ifndef CONSTANTBUFFERS_HPP
#define CONSTANTBUFFERS_HPP

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>


struct CameraBuffer
{
    glm::mat4 mViewMatrix;
    glm::mat4 mPerspectiveMatrix;
	
    glm::mat4 mInvertedCameraMatrix;
	
    glm::vec4 mFrameBufferSize;
};


struct SSAOBuffer
{
    std::array<glm::vec4, 16> mOffsets;
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

