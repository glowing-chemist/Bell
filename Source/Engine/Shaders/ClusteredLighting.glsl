
#define FROXEL_TILE_SIZE   32
#define LIGHTS_PER_FROXEL  16
#define DEPTH_SUBDIVISIONS 32
#define DEPTH_SUBDIVISION_FACTOR 1.2
#define E 2.71828

#include "Utilities.glsl"


// light Types:
// 0 - point
// 1 - spot
// 2 - square
// 3 - strip
struct Light
{
	vec4 position;
	vec4 direction; // direction for spotlight.
	vec4 albedo;
	float influence;
	uint type;
	vec2 padding;
};

struct AABB
{
	vec3 topLeft; // smallest
	vec3 bottomRight; // largest
};


uint getFroxelIndex(const uvec3 position, const uvec2 size)
{
	const uint rowSize = uint(ceil(float(size.x) / float(FROXEL_TILE_SIZE)));
	const uint columnSize = uint(ceil(float(size.y) / float(FROXEL_TILE_SIZE)));
	return position.x + (position.y * rowSize) + (position.z * rowSize * columnSize);
}


uvec3 getFroxelPosition(const uvec2 position, const float depth, const vec2 size, const float nearPlane, const float farPlane, const float FOV)
{
	const uvec2 xyFroxel = position / FROXEL_TILE_SIZE;

    float depthVS = lineariseDepth(depth, nearPlane, farPlane);
    depthVS *= (farPlane - nearPlane);
    depthVS += nearPlane;

    // Formula from original paper ( Clustered Deferred and forward shading, Ola et al.)
    // produces to many depth subdivisions for my liking.
	//const float Sy = size.y / FROXEL_TILE_SIZE; 
	//const float kn = floor((log(depthVS / nearPlane)) / (log(1 + (2 * tan(radians(FOV / 2.0f)) / Sy))));

	// Linear (used just for comparison).
	//float k = farPlane / float(DEPTH_SUBDIVISIONS);
	//float kn = floor(depthVS / k);

	// Version with a fixed (DEPTH_SUBDIVISIONS) number of subdivisions.
	const float k1 = (farPlane - nearPlane) / pow(DEPTH_SUBDIVISION_FACTOR, DEPTH_SUBDIVISIONS);
	const float kn = floor(log(depthVS / k1) / log(DEPTH_SUBDIVISION_FACTOR));

	return uvec3(xyFroxel, kn);
}


vec2 getWidthHeight(const float linearDepth, const float FOV, const vec2 framebufferSize)
{
	const float height = 2.0f * tan(radians(FOV) / 2.0f) * linearDepth;

	const float aspect = framebufferSize.x / framebufferSize.y;
	const float width = height * aspect;

	return vec2(width, height);
}


AABB getFroxelAABB(const uvec3 froxelPosition, const float FOV, const vec2 framebufferSize, const float nearPlane, const float farPlane)
{
	const float k1 = (farPlane - nearPlane) / pow(DEPTH_SUBDIVISION_FACTOR, DEPTH_SUBDIVISIONS);

	const float nearDepth = k1 * pow(E, log(DEPTH_SUBDIVISION_FACTOR) * froxelPosition.z);
	const vec2 nearWidthHeight = getWidthHeight(nearDepth, FOV, framebufferSize);
	vec3 nearViewSpace;
	nearViewSpace.xy = (((froxelPosition.xy * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight;
	nearViewSpace.z = nearDepth;

	const float farDepth = k1 * pow(E, log(DEPTH_SUBDIVISION_FACTOR) * (froxelPosition.z + 1));
	const vec2 farWidthHeight = getWidthHeight(farDepth, FOV, framebufferSize);
	vec3 farViewSpace;
	farViewSpace.xy = ((((froxelPosition.xy + uvec2(1, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight;
	farViewSpace.z = farDepth;

	return AABB(nearViewSpace, farViewSpace);
}


// Interscetion helpers
bool sphereAABBIntersection(const vec3 centre, const float radius, const AABB aabb)
{
	const float r2 = radius * radius;
	bvec3 lessThan = bvec3(centre.x < aabb.topLeft.x,
							centre.y < aabb.topLeft.y,
							centre.z < aabb.topLeft.z);
	const vec3 lessDistance = centre - aabb.topLeft;
	vec3 dmin = mix(vec3(0.0f), lessDistance * lessDistance, lessThan);

	bvec3 greaterThan = bvec3(centre.x > aabb.bottomRight.x,
								centre.y > aabb.bottomRight.y,
								centre.z > aabb.bottomRight.z);
	const vec3 greaterDistance = centre - aabb.bottomRight; 
	dmin += mix(vec3(0.0f), greaterDistance * greaterDistance, greaterThan);

	// Sum the results.
	dmin.x = dot(dmin, vec3(1.0f));

	return dmin.x <= r2;
}