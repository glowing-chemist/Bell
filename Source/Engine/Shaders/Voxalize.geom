#include "VertexOutputs.hlsl" 
#include "UniformBuffers.hlsl"

[[vk::binding(1)]]
ConstantBuffer<VoxelDimmensions> voxelDimmensions;

[maxvertexcount(3)]
void main(
	triangle GBufferVertOutput input[3],
	inout TriangleStream< VoxalizeGeometryOutput > outputStream
)
{
	VoxalizeGeometryOutput output[3];

	float3 facenormal = abs(input[0].normal + input[1].normal + input[2].normal);
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	for (uint i = 0; i < 3; ++i)
	{
		// World space -> Voxel grid space:
		output[i].position.xyz = (input[i].positionWS.xyz - voxelDimmensions.voxelCentreWS) * voxelDimmensions.recipVoxelSize;

		// Project onto dominant axis:
		if (maxi == 0)
		{
			output[i].position.xyz = output[i].position.zyx;
		}
		else if (maxi == 1)
		{
			output[i].position.xyz = output[i].position.xzy;
		}
	}


	// Expand triangle to get fake Conservative Rasterization:
	float2 side0N = normalize(output[1].position.xy - output[0].position.xy);
	float2 side1N = normalize(output[2].position.xy - output[1].position.xy);
	float2 side2N = normalize(output[0].position.xy - output[2].position.xy);
	output[0].position.xy += normalize(side2N - side0N);
	output[1].position.xy += normalize(side0N - side1N);
	output[2].position.xy += normalize(side1N - side2N);

	for (uint j = 0; j < 3; j++)
	{
		// Voxel grid space -> Clip space
		output[j].position.xy *= voxelDimmensions.recipVolumeSize;
		output[j].position.zw = 1;

		// Append the rest of the parameters as is:
		output[j].uv = input[j].uv;
		output[j].normal = input[j].normal;
		output[j].positionWS = input[j].positionWS;
		output[j].materialID = input[j].materialID;

		outputStream.Append(output[j]);
	}

	outputStream.RestartStrip();
}
