#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 positionVS;

layout(location = 0) out vec2 shadowMap;


void main()
{
	shadowMap.x = -positionVS.z;
	shadowMap.y = positionVS.z * positionVS.z;
}
