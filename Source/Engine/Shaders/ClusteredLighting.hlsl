
#define FROXEL_TILE_SIZE   32
#define LIGHTS_PER_FROXEL  16
#define DEPTH_SUBDIVISIONS 32
#define DEPTH_SUBDIVISION_FACTOR 1.2
#define E 2.71828

#include "Utilities.hlsl"
#include "PBR.hlsl"

#define LINEAR_SUBDIVISIONS 0

// light Types:
// 0 - point
// 1 - spot
// 2 - square
// 3 - strip
struct Light
{
	float3 position;
	float intensity;
	float3 direction; // direction for spotlight.
	float radius;
	float3 up;
	uint type;
	float4 albedo;
	float3 misc; // angle for spotlight and side lenght fo area.
	uint _padding;
};

struct AABB
{
	float4 min; // smallest
	float4 max; // largest
};

uint getFroxelIndex(const uint3 position, const uint2 size)
{
	const uint rowSize = uint(ceil(float(size.x) / float(FROXEL_TILE_SIZE)));
	const uint columnSize = uint(ceil(float(size.y) / float(FROXEL_TILE_SIZE)));
	return position.x + (position.y * rowSize) + (position.z * rowSize * columnSize);
}


uint3 getFroxelPosition(const uint2 position, const float depth, const float2 size, const float nearPlane, const float farPlane, const float FOV)
{
	const uint2 xyFroxel = position / FROXEL_TILE_SIZE;

    // Formula from original paper ( Clustered Deferred and forward shading, Ola et al.)
    // produces to many depth subdivisions for my liking.
	//const float Sy = size.y / FROXEL_TILE_SIZE; 
	//const float kn = floor((log(depthVS / nearPlane)) / (log(1 + (2 * tan(radians(FOV / 2.0f)) / Sy))));
#if LINEAR_SUBDIVISIONS
	// Linear (used just for comparison).
	float k = farPlane / float(DEPTH_SUBDIVISIONS);
	uint kn = uint(depth / k);
#else
	// Version with a fixed (DEPTH_SUBDIVISIONS) number of subdivisions.
	const float k1 = (farPlane - nearPlane) / pow(DEPTH_SUBDIVISION_FACTOR, DEPTH_SUBDIVISIONS);
	const float kn = floor(log(depth / k1) / log(DEPTH_SUBDIVISION_FACTOR));
#endif

	return uint3(xyFroxel, kn);
}


float2 getWidthHeight(const float linearDepth, const float FOV, const float2 framebufferSize)
{
	const float height = 2.0f * linearDepth * tan(radians(FOV) * 0.5f);

	const float aspect = framebufferSize.x / framebufferSize.y;
	const float width = height * aspect;

	return float2(width, height);
}


AABB getFroxelAABB(const uint3 froxelPosition, const float FOV, const float2 framebufferSize, const float nearPlane, const float farPlane)
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

	const float2 farWidthHeight = getWidthHeight(farDepth, FOV, framebufferSize);
	const float2 nearWidthHeight = getWidthHeight(nearDepth, FOV, framebufferSize);

	float3 nearVerticies[4];
	nearVerticies[0] = float3(((float2((froxelPosition.xy + uint2(0, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);
	nearVerticies[1] = float3(((float2((froxelPosition.xy + uint2(1, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);
	nearVerticies[2] = float3(((float2((froxelPosition.xy + uint2(0, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);
	nearVerticies[3] = float3(((float2((froxelPosition.xy + uint2(1, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * nearWidthHeight, -nearDepth);

	float3 farVerticies[4];
	farVerticies[0] = float3(((float2((froxelPosition.xy + uint2(0, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);
	farVerticies[1] = float3(((float2((froxelPosition.xy + uint2(1, 0)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);
	farVerticies[2] = float3(((float2((froxelPosition.xy + uint2(0, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);
	farVerticies[3] = float3(((float2((froxelPosition.xy + uint2(1, 1)) * FROXEL_TILE_SIZE) / framebufferSize) - 0.5f) * farWidthHeight, -farDepth);

	float3 nearViewSpace = float3(1000000.0f);
	float3 farViewSpace = float3(-1000000.0f);

	for(uint i = 0; i < 4; ++i)
	{
		nearViewSpace = min(nearViewSpace, nearVerticies[i]);
		nearViewSpace = min(nearViewSpace, farVerticies[i]);

		farViewSpace = max(farViewSpace, nearVerticies[i]);
		farViewSpace = max(farViewSpace, farVerticies[i]);
	}

	return AABB(float4(nearViewSpace, 1.0f), float4(farViewSpace, 1.0f));
}


void initializeLightState(out float3x3 minv, out float ltcAmp, out float3 dfg, Texture2D<float3> DFG, Texture2D<float4> LTCMat, Texture2D<float2> LTCAmp, SamplerState linearSampler, const float NoV, const float R)
{
	dfg = DFG.Sample(linearSampler, float2(NoV, R));

	const float4 t = LTCMat.Sample(linearSampler, float2(R, NoV));
    minv = float3x3(
            	float3(t.z,   0, t.w),
            	float3(   0, 1,   0),
            	float3(t.y,   0, t.x)
        		);

    ltcAmp = LTCAmp.Sample(linearSampler, float2(R, NoV));
}


float4 pointLightContribution(const Light light, 
							const float4 positionWS, 
							const float3 view,
							const MaterialInfo material,
							const float3 dfg)
{
    const float3 lightDir = light.position - positionWS.xyz;
	const float lightDistance = length(lightDir);

    const float falloff = pow(saturate(1 - pow(lightDistance / light.radius, 4.0f)), 2.0f) / ((lightDistance * lightDistance) + 1); 
	const float3 radiance = light.albedo.xyz * light.intensity * falloff;

    const float3 diffuse = calculateDiffuseDisney(material, radiance, dfg);
    const float3 specular = calculateSpecular(material, radiance, dfg);

    return float4(diffuse + specular, 1.0f);
}


float4 pointLightContributionDiffuse(const Light light, 
									const float4 positionWS, 
									const MaterialInfo material,
									const float3 dfg)
{
    const float3 lightDir = light.position - positionWS.xyz;
	const float lightDistance = length(lightDir);

    const float falloff = pow(saturate(1 - pow(lightDistance / light.radius, 4.0f)), 2.0f) / ((lightDistance * lightDistance) + 1); 
	const float3 radiance = light.albedo.xyz * light.intensity * falloff;

    const float3 diffuse = calculateDiffuseDisney(material, radiance, dfg);

    return float4(diffuse, 1.0f);
}


float4 spotLightContribution(const Light light, 
							const float4 positionWS, 
							const float3 view,
							const MaterialInfo material,
							const float3 dfg)
{
	if(acos(dot(positionWS - light.position, light.direction)) < radians(light.misc.x))
		return float4(0.0f, 0.0f, 0.0f, 0.0f);

    const float3 lightDir = light.position - positionWS.xyz;
	const float lightDistance = length(lightDir);

    const float falloff = pow(saturate(1 - pow(lightDistance / light.radius, 4.0f)), 2.0f) / ((lightDistance * lightDistance) + 1); 
	const float3 radiance = light.albedo.xyz * light.intensity * falloff;

    const float3 diffuse = calculateDiffuseDisney(material, radiance, dfg);
    const float3 specular = calculateSpecular(material, radiance, dfg);

    return float4(diffuse + specular, 1.0f);
}


float4 spotLightContributionDiffuse(const Light light, 
									const float4 positionWS, 
									const MaterialInfo material,
									const float3 dfg)
{
	if(acos(dot(positionWS - light.position, light.direction)) < radians(light.misc.x))
		return float4(0.0f, 0.0f, 0.0f, 0.0f);

    const float3 lightDir = light.position - positionWS.xyz;
	const float lightDistance = length(lightDir);

    const float falloff = pow(saturate(1 - pow(lightDistance / light.radius, 4.0f)), 2.0f) / ((lightDistance * lightDistance) + 1); 
	const float3 radiance = light.albedo.xyz * light.intensity * falloff;

    const float3 diffuse = calculateDiffuseDisney(material, radiance, dfg);

    return float4(diffuse, 1.0f);
}


// Area light functions From Eric Heitz.
float IntegrateEdge(float3 v1, float3 v2)
{
    float cosTheta = dot(v1, v2);
    float theta = acos(cosTheta);    
    float res = cross(v1, v2).z * ((theta > 0.001) ? theta / sin(theta) : 1.0);

    return res;
}

// For sqaure area lights
float3 LTC_Evaluate(float3 N, float3 V, float3 P, float3x3 Minv, float3 points[4])
{
    // construct orthonormal basis around N
    float3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = mul(Minv, float3x3(T1, T2, N));

    // polygon (allocate 5 vertices for clipping)
    float3 L[4];
    L[0] = mul(Minv, (points[0] - P));
    L[1] = mul(Minv, (points[1] - P));
    L[2] = mul(Minv, (points[2] - P));
    L[3] = mul(Minv, (points[3] - P));

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

    return float3(sum);
}

// Strip light imple from Eric Heitz.
float Fpo(float d, float l)
{
    return l/(d*(d*d + l*l)) + atan(l/d)/(d*d);
}

float Fwt(float d, float l)
{
    return l*l/(d*(d*d + l*l));
}

float3x3 invert3x3(float3x3 m)
{
	const float D = determinant(m);
 	float3x3 adj;

	 adj[0][0] = determinant(float2x2(m[1][1], m[1][2], m[2][1], m[2][2]));
	 adj[0][1] = -determinant(float2x2(m[0][1], m[0][2], m[2][1], m[2][2]));
	 adj[0][2] = determinant(float2x2(m[0][1], m[0][2], m[1][1], m[1][2]));

	 adj[1][0] = -determinant(float2x2(m[1][0], m[1][2], m[2][0], m[2][2]));
	 adj[1][1] = determinant(float2x2(m[0][0], m[0][2], m[2][0], m[2][2]));
	 adj[1][2] = -determinant(float2x2(m[0][0], m[0][2], m[1][0], m[1][2]));

	 adj[2][0] = determinant(float2x2(m[1][0], m[1][1], m[2][0], m[2][1]));
	 adj[2][1] = -determinant(float2x2(m[0][0], m[0][1], m[2][0], m[2][1]));
	 adj[2][2] = determinant(float2x2(m[0][0], m[0][1], m[1][0], m[1][1]));

	 return adj *  (1.0f / D);
}

float I_diffuse_line(float3 p1, float3 p2)
{
    // tangent
    float3 wt = normalize(p2 - p1);

    // clamping
    if (p1.z <= 0.0 && p2.z <= 0.0) return 0.0;
    if (p1.z < 0.0) p1 = (+p1*p2.z - p2*p1.z) / (+p2.z - p1.z);
    if (p2.z < 0.0) p2 = (-p1*p2.z + p2*p1.z) / (-p2.z + p1.z);

    // parameterization
    float l1 = dot(p1, wt);
    float l2 = dot(p2, wt);

    // shading point orthonormal projection on the line
    float3 po = p1 - l1*wt;

    // distance to line
    float d = length(po);

    // integral
    float I = (Fpo(d, l2) - Fpo(d, l1)) * po.z +
              (Fwt(d, l2) - Fwt(d, l1)) * wt.z;
    return I / PI;
}

float I_ltc_line(float3 p1, float3 p2, float3x3 Minv)
{
    // transform to diffuse configuration
    float3 p1o = mul(Minv, p1);
    float3 p2o = mul(Minv, p2);
    float I_diffuse = I_diffuse_line(p1o, p2o);

    // width factor
    float3 ortho = normalize(cross(p1, p2));
    float w =  1.0 / length(mul(invert3x3(transpose(Minv)), ortho));

    return w * I_diffuse;
}

// For strip lights.
float3 LTC_Evaluate(float3 N, float3 V, float3 P, float R, float3x3 Minv, float3 linePoints[2])
{
    // construct orthonormal basis around N
    float3 T1, T2;
    T1 = normalize(V - N*dot(V, N));
    T2 = cross(N, T1);

    float3x3 B = transpose(float3x3(T1, T2, N));

    float3 p1 = mul(B, linePoints[0] - P);
    float3 p2 = mul(B, linePoints[1] - P);

    float Iline = R * I_ltc_line(p1, p2, Minv);
    return float3(min(1.0, Iline));
}

float4 areaLightContribution(const Light light, 
							const float4 positionWS, 
							const float3 view,
							const MaterialInfo material,
							const float3x3 Minv,
							const float amp)
{
    const float3 rightVector = cross(light.up.xyz, light.direction.xyz); 

    // Calculate the 4 corners of the square area light in WS.
    float3 points[4];
    points[0] = light.position.xyz + 0.5f * ((light.misc.x * -rightVector) + (light.misc.y * light.up.xyz));
    points[1] = light.position.xyz + 0.5f * ((light.misc.x * rightVector) + (light.misc.y * light.up.xyz));
    points[2] = light.position.xyz + 0.5f * ((light.misc.x * rightVector) - (light.misc.y * light.up.xyz));
    points[3] = light.position.xyz + 0.5f * ((light.misc.x * -rightVector) - (light.misc.y * light.up.xyz));

    float3 spec = LTC_Evaluate(material.normal.xyz, view, positionWS.xyz, Minv, points);
    spec *= amp;
        
    const float3 diff = LTC_Evaluate(material.normal.xyz, view, positionWS.xyz, float3x3(float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f)), points); 
        
    float3 colour  = light.intensity * light.albedo.xyz * (spec * material.specularRoughness.xyz + diff * material.diffuse);

    return float4(colour, 1.0f);
}


float4 areaLightContributionDiffuse(const Light light, 
									const float4 positionWS, 
									const float3 view,
									const MaterialInfo material,
									const float3x3 Minv)
{
    const float3 rightVector = cross(light.up.xyz, light.direction.xyz); 

    // Calculate the 4 corners of the square area light in WS.
    float3 points[4];
    points[0] = light.position.xyz + (light.misc.x / 2.0f) * (-rightVector + light.up.xyz);
    points[1] = light.position.xyz + (light.misc.x / 2.0f) * (rightVector + light.up.xyz);
    points[2] = light.position.xyz + (light.misc.x / 2.0f) * (rightVector - light.up.xyz);
    points[3] = light.position.xyz + (light.misc.x / 2.0f) * (-rightVector - light.up.xyz);
        
    const float3 diff = LTC_Evaluate(material.normal.xyz, view, positionWS.xyz, float3x3(float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f)), points); 
        
    float3 colour  = light.intensity * (light.albedo.xyz * diff * material.diffuse);

    return float4(colour, 1.0f);
}

float4 stripLightContribution(const Light light, 
							const float4 positionWS, 
							const float3 view,
							const MaterialInfo material,
							const float3x3 Minv,
							const float amp)
{
    // Calculate the 2 end of the light in WS.
    float3 points[2];
    points[0] = light.position.xyz + 0.5f * light.misc.y * light.direction;
    points[1] = light.position.xyz - 0.5f * light.misc.y * light.direction;

    float3 spec = LTC_Evaluate(material.normal.xyz, view, positionWS.xyz, light.misc.x, Minv, points);
    spec *= amp;
        
    const float3 diff = LTC_Evaluate(material.normal.xyz, view, positionWS.xyz, light.misc.x, float3x3(float3(1.0f, 0.0f, 0.0f), float3(0.0f, 1.0f, 0.0f), float3(0.0f, 0.0f, 1.0f)), points); 
        
    float3 colour  = light.intensity * light.albedo.xyz * (spec * material.specularRoughness.xyz + diff * material.diffuse);

    return float4(colour, 1.0f);
}

// Interscetion helpers
bool sphereAABBIntersection(const float3 centre, const float radius, const AABB aabb)
{
	const float r2 = radius * radius;
	const bool3 lessThan = bool3(centre.x < aabb.min.x,
							centre.y < aabb.min.y,
							centre.z < aabb.min.z);
	const float3 lessDistance = aabb.min.xyz - centre;
	float3 dmin = lerp(float3(0.0f), lessDistance * lessDistance, lessThan);

	const bool3 greaterThan = bool3(centre.x > aabb.max.x,
								centre.y > aabb.max.y,
								centre.z > aabb.max.z);
	const float3 greaterDistance = centre - aabb.max.xyz;
	dmin += lerp(float3(0.0f), greaterDistance * greaterDistance, greaterThan);

	// Sum the results.
	dmin.x = dot(dmin, float3(1.0f));

	return dmin.x <= r2;
}


bool AABBAABBIntersection(const AABB a, const AABB b)
{
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
         (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
         (a.min.z <= b.max.z && a.max.z >= b.min.z);
}


bool spotLightAABBIntersection(const float3 centre, const float3 direction, const float angle, const float radius, const AABB aabb)
{
	const float3 toNear = normalize(aabb.min.xyz - centre);
	const float3 toFar = normalize(aabb.max.xyz - centre);

	return ((acos(dot(toNear, direction)) < radians(angle)) || (acos(dot(toFar, direction)) < radians(angle))) && sphereAABBIntersection(centre, radius, aabb);
}


bool areaLightAABBIntersection(const float3 centre, const float3 normal, const float radius, const AABB aabb)
{
	const bool intersect = sphereAABBIntersection(centre, radius, aabb);

	return intersect;
}


bool stripLightAABBIntersection(const float3 centre, const float radius, const AABB aabb)
{
	const bool intersect = sphereAABBIntersection(centre, radius, aabb);

	return intersect;
}
