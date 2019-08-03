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

layout(binding = 0) uniform UniformBufferObject 
{    
    CameraBuffer camera;
}; 

layout(binding = 1) uniform Uniformtransformation
{    
    mat4 transformation;
} instanceTransformations[];

void main()
{
	const mat4 transFormation = camera.perspective * camera.view * instanceTransformations[gl_InstanceID].transformation;

	gl_Position = transFormation * position;
	outNormals = instanceTransformations[gl_InstanceID].transformation * normals;
	outPosition = gl_Position;
}