#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in uint material;

out gl_PerVertex {
    vec4 gl_Position;
};


layout(push_constant) uniform pushModelMatrix
{
	mat4 model;
} push_constants;


void main()
{
	gl_Position = push_constants.model * position;
}