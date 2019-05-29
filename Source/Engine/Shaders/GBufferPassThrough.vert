#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normals;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out vec4 outPosition;

out gl_PerVertex {
    vec4 gl_Position;
};


layout(binding = 0) uniform UniformBufferObject 
{    
    mat4 model;    
    mat4 view;    
    mat4 perspective;    
} camera; 


void main()
{
	const mat4 transFormation = camera.perspective * camera.view * camera.model;

	gl_Position = transFormation * position;
	outNormals = camera.model * normals;
	outPosition = gl_Position;
}
