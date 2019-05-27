#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec4 normals;
layout(location = 1) in vec4 albedo;
layout(location = 2) in float materialID;
layout(location = 3) in vec2 uv;
layout(location = 4) in vec4 position;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out vec4 outAlbedo;
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec2 outUV;
layout(location = 4) out float outMaterial;


void main()
{
	outNormals = (normals + 1.0f) * 0.5f;
	outAlbedo = albedo;
	outPosition = (position + 1.0f) * 0.5f;
	outUV = uv;
	outMaterial = materialID;
}