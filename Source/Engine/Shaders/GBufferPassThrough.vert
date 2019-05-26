#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 normals;
layout(location = 3) in vec4 albedo;
layout(location = 4) in float material;


layout(location = 0) out vec4 outNormals;
layout(location = 1) out vec4 outAlbedo;
layout(location = 2) out float outMaterialID;


out gl_PerVertex {
    vec4 gl_Position;
};


layout(binding = 0) uniform UniformBufferObject {    
    mat4 model;    
    mat4 view;    
    mat4 perspective;    
} camera; 


void main()
{
	const mat4 transFormation = camera.perspective * camera.view * camera.model;

	gl_Position = transFormation * position;
	outNormals = camera.model * normals;
	outAlbedo = albedo;
	outMaterialID = material;
}
