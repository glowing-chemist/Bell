
#define FROXEL_TILE_SIZE   32
#define LIGHTS_PER_FROXEL  16
#define DEPTH_SUBDIVISIONS 32
#define DEPTH_SUBDIVISION_FACTOR 1.2

#include "Utilities.glsl"


uint getFroxelIndex(const uvec3 position, const uvec2 size)
{
	return position.x + (position.y * (size.x / FROXEL_TILE_SIZE)) + (position.z * ((size.x * size.y) / (FROXEL_TILE_SIZE * FROXEL_TILE_SIZE)));
}


uvec3 getFroxelPosition(const vec2 uv, const float depth, const vec2 size, const float nearPlane, const float farPlane, const float FOV)
{
	const vec2 xyFroxel = floor((uv * size) / FROXEL_TILE_SIZE);

    float depthVS = lineariseDepth(depth, nearPlane, farPlane);
    depthVS *= farPlane;

    // Formula from original paper ( Clustered Deferred and forward shading, Ola et al.)
    // produces to many depth subdivisions for my liking.
	//const float Sy = size.y / FROXEL_TILE_SIZE; 
	//const float kn = floor((log(depthVS / nearPlane)) / (log(1 + (2 * tan(radians(FOV / 2.0f)) / Sy))));

	// Version with a fixed (DEPTH_SUBDIVISIONS) number if subdivisions.
	const float k1 = (farPlane - nearPlane) / pow(DEPTH_SUBDIVISION_FACTOR, DEPTH_SUBDIVISIONS);
	const float kn = floor(log(depthVS / k1) / log(DEPTH_SUBDIVISION_FACTOR));

	return uvec3(xyFroxel, kn);
}
