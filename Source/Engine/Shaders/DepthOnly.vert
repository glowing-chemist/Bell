#include "VertexOutputs.hlsl"
#include "UniformBuffers.hlsl"


[[vk::binding(0)]]
ConstantBuffer<CameraBuffer> camera;

struct mesh_matrix
{
	float4x4 mat;
};
[[vk::push_constant]]
ConstantBuffer<mesh_matrix> model;


DepthOnlyOutput main(Vertex vertex)
{
	DepthOnlyOutput output;

	output.position = mul(mul(camera.viewProj, model.mat), vertex.position);
	output.uv = vertex.uv;
	output.materialID = vertex.materialID;

	return output;
}