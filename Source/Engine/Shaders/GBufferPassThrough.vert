#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normals;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out vec4 outPosition;

out gl_PerVertex {
    vec4 gl_Position;
};


layout(push_constant) uniform pushModelMatrix
{
	mat4 model;
} push_constants;


layout(binding = 0) uniform UniformBufferObject 
{    
    CameraBuffer camera;
}; 


void main()
{
	const mat4 transFormation = camera.perspective * camera.view * push_constants.model;

	gl_Position = transFormation * position;
	outNormals = push_constants.model * normals;
	outPosition = gl_Position;
}
