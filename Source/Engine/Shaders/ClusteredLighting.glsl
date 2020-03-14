
#define FROXEL_TILE_SIZE   32
#define LIGHTS_PER_FROXEL  16
#define DEPTH_SUBDIVISIONS 32
#define DEPTH_SUBDIVISION_FACTOR 1.2
#define E 2.71828

#include "Utilities.glsl"
#include "PBR.glsl"

#define LINEAR_SUBDIVISIONS 0

// light Types:
// 0 - point
// 1 - spot
// 2 - square
// 3 - strip
struct Light
{
	vec4 position;
	vec4 direction; // direction for spotlight.
	vec4 up;
	vec4 albedo;
	float intensity;
	float radius;
	uint type;
	float misc; // angle for spotlight and side lenght fo area.
};

struct AABB
{
	vec4 min; // smallest
	vec4 max; // largest
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

    float depthVS = lineariseReverseDepth(depth, nearPlane, farPlane);
    depthVS *= (farPlane - nearPlane);
    depthVS += nearPlane;

    // Formula from original paper ( Clustered Deferred and forward shading, Ola et al.)
    // produces to many depth subdivisions for my liking.
	//const float Sy = size.y / FROXEL_TILE_SIZE; 
	//const float kn = floor((log(depthVS / nearPlane)) / (log(1 + (2 * tan(radians(FOV / 2.0f)) / Sy))));
#if LINEAR_SUBDIVISIONS
	// Linear (used just for comparison).
	float k = farPlane / float(DEPTH_SUBDIVISIONS);
	uint kn = uint(depthVS / k);
#else
	// Version with a fixed (DEPTH_SUBDIVISIONS) number of subdivisions.
	const float k1 = (farPlane - nearPlane) / pow(DEPTH_SUBDIVISION_FACTOR, DEPTH_SUBDIVISIONS);
	const float kn = floor(log(depthVS / k1) / log(DEPTH_SUBDIVISION_FACTOR));
#endif

	return uvec3(xyFroxel, kn);
}


vec2 getWidthHeight(const float linearDepth, const float FOV, const vec2 framebufferSize)
{
	const float height = 2.0f * linearDepth * tan(radians(FOV) * 0.5f);

	const float aspect = framebufferSize.x / framebufferSize.y;
	const float width = height * aspect;

	return vec2(width, height);
}


AABB getFroxelAABB(const uvec3 froxelPosition, const float FOV, const vec2 framebufferSize, const float nearPlane, const float farPlane)
{
#if LINEAR_SUBDIVISIONS
	float k1 = farPlane / float(DEPTH_SUBDIVISIONS);

	const float nearDepth = k1 * froxelPosition.z;
	const float farDepth = k1 * (froxelPosition.z + 1);
#else
	const float k1 = (farPlane - nearPlane) / pow(DEPTH_SUBDIVISION_FACTOR, DEPTH_SUBDIVISIONS);

	const float nearDepth = k1 * pow(E, log(DEPTH_SUBDIVISION_FACTOR) * froxelPosition.z);
	const float farDepth = k1 * pow(E, log(DEPTH_SUBDIVISION_FACTOR) * (froxelPosition.z + 1));
#endif

	const vec2 farWidthHeight = getWidthHeight(farDepth, FOV, framebufferSize);
	const vec2 nearWidthHeight = getWidthHeight(nearDepth, FOV, framebufferSize);

	vec3 nearVerticies[4];
	nearVerticies[0] = vec3(((vec2((froxelPosition.xy + uvec2(0, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);
	nearVerticies[1] = vec3(((vec2((froxelPosition.xy + uvec2(1, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);
	nearVerticies[2] = vec3(((vec2((froxelPosition.xy + uvec2(0, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);
	nearVerticies[3] = vec3(((vec2((froxelPosition.xy + uvec2(1, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);

	vec3 farVerticies[4];
	farVerticies[0] = vec3(((vec2((froxelPosition.xy + uvec2(0, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);
	farVerticies[1] = vec3(((vec2((froxelPosition.xy + uvec2(1, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);
	farVerticies[2] = vec3(((vec2((froxelPosition.xy + uvec2(0, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);
	farVerticies[3] = vec3(((vec2((froxelPosition.xy + uvec2(1, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);

	vec3 nearViewSpace = vec3(1000000.0f);
	vec3 farViewSpace = vec3(-1000000.0f);

	for(uint i = 0; i < 4; ++i)
	{
		nearViewSpace = min(nearViewSpace, nearVerticies[i]);
		nearViewSpace = min(nearViewSpace, farVerticies[i]);

		farViewSpace = max(farViewSpace, nearVerticies[i]);
		farViewSpace = max(farViewSpace, farVerticies[i]);
	}

	return AABB(vec4(nearViewSpace, 1.0f), vec4(farViewSpace, 1.0f));
}


void initializeLightState(out mat3 minv, out float ltcAmp, out vec2 f_ab, texture2D DFG, texture2D LTCMat, texture2D LTCAmp, sampler linearSampler, const float NoV, const float R)
{
	f_ab = texture(sampler2D(DFG, linearSampler), vec2(NoV, R)).xy;

	const vec4 t = texture(sampler2D(LTCMat, linearSampler), vec2(R, NoV));
    minv = mat3(
            	vec3(  1,   0, t.y),
            	vec3(  0, t.z,   0),
            	vec3(t.w,   0, t.x)
        		);

    ltcAmp = texture(sampler2D(LTCAmp, linearSampler), vec2(R, NoV)).x;
}


vec4 pointLightContribution(const Light light, 
							const vec4 positionWS, 
							const vec3 view,
							const vec3 N,
							const float metalness, 
							const float roughness, 
							const vec3 baseAlbedo, 
							const vec2 f_ab)
{
    const vec4 lightDir = light.position - positionWS;
	const float lightDistance = length(lightDir);

    const float falloff = pow(clamp(1 - pow(lightDistance / light.radius, 4.0f), 0.0f, 1.0f), 2.0f) / ((lightDistance * lightDistance) + 1); 
	const vec3 radiance = light.albedo.xyz * light.intensity * falloff;

    const vec3 diffuse = calculateDiffuseLambert(baseAlbedo.xyz, metalness, radiance);
    const vec3 specular = calculateSpecular(roughness * roughness, N, view, metalness, baseAlbedo.xyz, radiance, f_ab);

    return vec4(diffuse + specular, 1.0f);
}


// Area light functions.
float IntegrateEdge(vec3 v1, vec3 v2)
{
    float cosTheta = dot(v1, v2);
    float theta = acos(cosTheta);    
    float res = cross(v1, v2).z * ((theta > 0.001) ? theta / sin(theta) : 1.0);

    return res;
}

vec3 LTC_Evaluate(
    vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4])
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N*dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));

    // polygon (allocate 5 vertices for clipping)
    vec3 L[4];
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    // Don't perform clipping (it's expensive and difficult to implement)
    // Doesn't have a large impact so ignore for now.

    // project onto sphere
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    // integrate
    float sum = 0.0;

    sum += IntegrateEdge(L[0], L[1]);
    sum += IntegrateEdge(L[1], L[2]);
    sum += IntegrateEdge(L[2], L[3]);
    sum += IntegrateEdge(L[3], L[0]);

    sum = max(0.0, sum);

    return vec3(sum);
}

vec4 areaLightContribution(const Light light, 
							const vec4 positionWS, 
							const vec3 view,
							const vec3 N,
							const float metalness, 
							const float roughness, 
							const vec3 baseAlbedo, 
							const mat3 Minv,
							const float amp)
{
    const vec3 rightVector = cross(light.up.xyz, light.direction.xyz); 

    // Calculate the 4 corners of the square area light in WS.
    vec3 points[4];
    points[0] = light.position.xyz + (light.misc / 2.0f) * (-rightVector + light.up.xyz);
    points[1] = light.position.xyz + (light.misc / 2.0f) * (rightVector + light.up.xyz);
    points[2] = light.position.xyz + (light.misc / 2.0f) * (rightVector - light.up.xyz);
    points[3] = light.position.xyz + (light.misc / 2.0f) * (-rightVector - light.up.xyz);

    vec3 spec = LTC_Evaluate(N, view, positionWS.xyz, Minv, points);
    spec *= amp;
        
    const vec3 diff = LTC_Evaluate(N, view, positionWS.xyz, mat3(1), points); 

    const vec3 diffuseColor = baseAlbedo * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalness);
	const vec3 F0 = mix(vec3(DIELECTRIC_SPECULAR), baseAlbedo, metalness);
        
    vec3 colour  = light.intensity * (light.albedo.xyz * spec * F0 + light.albedo.xyz * diff * diffuseColor);
    colour /= 2.0 * PI;

    return vec4(colour, 1.0f);
}


// Interscetion helpers
bool sphereAABBIntersection(const vec3 centre, const float radius, const AABB aabb)
{
	const float r2 = radius * radius;
	const bvec3 lessThan = bvec3(centre.x < aabb.min.x,
							centre.y < aabb.min.y,
							centre.z < aabb.min.z);
	const vec3 lessDistance = aabb.min.xyz - centre;
	vec3 dmin = mix(vec3(0.0f), lessDistance * lessDistance, lessThan);

	const bvec3 greaterThan = bvec3(centre.x > aabb.max.x,
								centre.y > aabb.max.y,
								centre.z > aabb.max.z);
	const vec3 greaterDistance = centre - aabb.max.xyz;
	dmin += mix(vec3(0.0f), greaterDistance * greaterDistance, greaterThan);

	// Sum the results.
	dmin.x = dot(dmin, vec3(1.0f));

	return dmin.x <= r2;
}


bool AABBAABBIntersection(const AABB a, const AABB b)
{
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
         (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
         (a.min.z <= b.max.z && a.max.z >= b.min.z);
}


bool spotLightAABBIntersection(const vec3 centre, const vec3 direction, const float angle, const float radius, const AABB aabb)
{
	const vec3 toNear = normalize(aabb.min.xyz - centre);
	const vec3 toFar = normalize(aabb.max.xyz - centre);

	return ((acos(dot(toNear, direction)) < radians(angle)) || (acos(dot(toFar, direction)) < radians(angle))) && sphereAABBIntersection(centre, radius, aabb);
}


bool areaLightAABBIntersection(const vec3 centre, const vec3 normal, const vec3 up, const float radius, const AABB aabb)
{
	const vec3 right = cross(up, normal);
	const vec3 minCorner = centre + (radius * 0.5f * (right - up));
	const vec3 maxCorner = centre + (radius * 0.5f * (up - right)) + normal * radius;

	const vec3 aabbMin = min(minCorner, maxCorner);
	const vec3 aabbMax = max(maxCorner, minCorner);

	return AABBAABBIntersection(AABB(vec4(aabbMin, 1.0f), vec4(aabbMax, 1.0f)), aabb);
}
