#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "MeshAttributes.glsl"

layout(location = 0) in float4 positionVS;
layout(location = 1) in float2 uv;
layout(location = 2) in flat uint materialID;

layout(location = 0) out float2 shadowMap;


layout(set = 0, binding = 1) uniform sampler linearSampler;

layout(set = 1, binding = 0) uniform texture2D materials[];


void main()
{
	{
		const float alpha = texture(sampler2D(materials[nonuniformEXT(materialID * 4)], linearSampler), uv).w;
		if(alpha == 0.0f)
			discard;
	}

	shadowMap.x = -positionVS.z;
	shadowMap.y = positionVS.z * positionVS.z;
}
