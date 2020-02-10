#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "UniformBuffers.glsl"


layout(location = 0) out vec3 UV;


out gl_PerVertex {
    vec4 gl_Position;
};


layout(binding = 0) uniform UniformBufferObject {    
    CameraBuffer camera;    
};


void main()
{
	vec2 triangleUVs = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	vec4 screenSpacePosition = vec4(triangleUVs * 2.0f + -1.0f, 1.0f, 1.0f);

	// calculate the cubemap texture coords
	vec4 worldSPacePerspective = camera.invertedViewProj * screenSpacePosition;
	worldSPacePerspective /= worldSPacePerspective.w;
	UV = normalize((worldSPacePerspective.xyz) - camera.position);
	gl_Position = screenSpacePosition;
}
