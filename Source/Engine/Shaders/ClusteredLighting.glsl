
#define FROXEL_TILE_SIZE  32
#define LIGHTS_PER_FROXEL 16


float lineariseDepth(const float depth, const float near, const float far)
{
	return  (2.0f * near) / (far + near - depth * (far - near));
}


uint getFroxelIndex(const uvec3 position)
{
	return 0;
}


uvec3 getFroxelPosition(const vec2 uv, const float depth, const vec2 size, const float nearPlane, const float farPlane, const float FOV)
{
	const vec2 xyFroxel = floor((uv * size) / FROXEL_TILE_SIZE);

    float depthVS = lineariseDepth(depth, nearPlane, farPlane);
    depthVS *= (nearPlane + farPlane);

	const float Sy = size.y / FROXEL_TILE_SIZE;
	const float k = floor((log(depthVS / nearPlane)) / (log(1 + (2 * tan(radians(FOV / 2.0f)) / Sy))));

	return uvec3(xyFroxel, k);
}
