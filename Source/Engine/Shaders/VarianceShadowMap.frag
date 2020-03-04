#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable


layout(location = 0) in vec4 positionVS;
layout(location = 1) in vec2 uv;
layout(location = 2) in flat uint materialID;

layout(location = 0) out vec2 shadowMap;


layout(set = 0, binding = 1) uniform sampler linearSampler;

layout(set = 1, binding = 0) uniform texture2D materials[];


void main()
{
	const float alpha = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler), uv).w;
	if(alpha == 0.0f)
		discard;

	shadowMap.x = -positionVS.z;
	shadowMap.y = positionVS.z * positionVS.z;
}
