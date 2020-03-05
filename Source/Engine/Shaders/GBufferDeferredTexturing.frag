#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 normals;
layout(location = 1) in flat uint materialID;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec2 velocity;

layout(location = 0) out vec3 outNormals;
layout(location = 1) out vec4 outUV;
layout(location = 2) out uint outMaterial;
layout(location = 3) out vec2 outVelocity;


void main()
{
	outNormals = (normals.xyz + 1.0f) * 0.5f;

	// calculate the partial derivitves of the uvs so that we can do manual LOD selection
	// in the lighting shaders. Will also help us calculate tangent space matracies later.
	vec4 uvWithDerivitives;
	uvWithDerivitives.xy = uv;

	// Pack the derivitives in to a single float this should still give us plenty of precision.
	uint packedXDerivitives = packHalf2x16(dFdxFine(uv));
	uint packedYDerivitives = packHalf2x16(dFdyFine(uv));

	uvWithDerivitives.z = uintBitsToFloat(packedXDerivitives);
	uvWithDerivitives.w = uintBitsToFloat(packedYDerivitives);

	outUV = uvWithDerivitives;
	outMaterial = materialID;
	outVelocity = (velocity * 0.5f) + 0.5f;
}