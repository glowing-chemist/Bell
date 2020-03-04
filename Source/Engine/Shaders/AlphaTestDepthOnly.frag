#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable


layout(location = 0) in vec2 uv;
layout(location = 1) in flat uint materialID;

layout(set = 0, binding = 1) uniform sampler linearSampler;

layout(set = 1, binding = 0) uniform texture2D materials[];

void main()
{
	// Just perform an alpha test.
	const float alpha = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler), uv).w;
	if(alpha == 0.0f)
		discard;
}