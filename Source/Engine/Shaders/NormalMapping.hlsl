

// Utility functions for normal mapping
// view vector is camera.pos - fragment.pos;
float3x3 tangentSpaceMatrix(const float3 vertNormal, const float3 view, const float4 uvDerivitives)
{
	// we don't/can't compute these here as we're doing defered textureing
	const float2 uvDx = uvDerivitives.xy;
	const float2 uvDy = uvDerivitives.zw;

	const float3 viewDx = ddx_fine(view);
	const float3 viewDy = ddy_fine(view);

	 // solve the linear system 
	 float3 dp2perp = cross(viewDy, vertNormal);
	 float3 dp1perp = cross(vertNormal, viewDx);
	 float3 tangent = dp2perp * uvDx.x + dp1perp * uvDy.x;
	 float3 bitangent = dp2perp * uvDx.y + dp1perp * uvDy.y;
	 
	 // construct a scale-invariant frame
	 float invmax = rsqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));

	 return float3x3(tangent * invmax, bitangent * invmax, vertNormal);
}


float3 remapNormals(const float3 N)
{
	return (N - 0.5f) * 2.0;
}


float2 remapNormals(const float2 N)
{
	return (N - 0.5f) * 2.0;
}


float reconstructNormalAxis(const float2 N)
{
	return sqrt(1.0f - dot(N.xy, N.xy));
}
