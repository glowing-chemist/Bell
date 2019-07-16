#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec4 normals;
layout(location = 1) in vec4 position;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out vec4 outAlbedo;
layout(location = 2) out vec4 outSpecular;


void main()
{
	outNormals = (normals + 1.0f) * 0.5f;
	outAlbedo = vec4(0.3f, 0.3f, 0.3f, 1.0f);
	outSpecular = (position + 1.0f) * 0.5f;
}