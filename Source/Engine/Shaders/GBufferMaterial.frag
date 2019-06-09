#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec4 normals;
layout(location = 1) in flat uint materialID;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec4 position;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out vec4 outPosition;
layout(location = 2) out vec4 outUV;
layout(location = 3) out uint outMaterial;


void main()
{
	outNormals = (normals + 1.0f) * 0.5f;
	outPosition = (position + 1.0f) * 0.5f;

	// calculate the partial derivitves of the uvs so that we can do manual LOD selection
	// in the lighting shaders. Will also help us calculate tangent space matracies later.
	vec4 uvWithDerivitives;
	uvWithDerivitives.xy = uv;

	uvWithDerivitives.z = dFdx(uv.x);
	uvWithDerivitives.w = dFdy(uv.y);

	outUV = uvWithDerivitives;
	outMaterial = materialID;
}