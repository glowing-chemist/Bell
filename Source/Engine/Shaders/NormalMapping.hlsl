

// Utility functions for normal mapping
// view vector is camera.pos - fragment.pos;
mat3 tangentSpaceMatrix(const vec3 vertNormal, const vec3 view, const vec4 uvDerivitives)
{
	// we don't/can't compute these here as we're doing defered textureing
	const vec2 uvDx = uvDerivitives.xy;
	const vec2 uvDy = uvDerivitives.zw;

	const vec3 viewDx = dFdxFine(view);
	const vec3 viewDy = dFdyFine(view);

	 // solve the linear system 
	 vec3 dp2perp = cross(viewDy, vertNormal);
	 vec3 dp1perp = cross(vertNormal, viewDx);
	 vec3 tangent = dp2perp * uvDx.x + dp1perp * uvDy.x;
	 vec3 bitangent = dp2perp * uvDx.y + dp1perp * uvDy.y;
	 
	 // construct a scale-invariant frame
	 float invmax = inversesqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));

	 return mat3(tangent * invmax, bitangent * invmax, vertNormal);
}


vec3 remapNormals(const vec3 N)
{
	return (N - 0.5f) * 2.0;
}


vec2 remapNormals(const vec2 N)
{
	return (N - 0.5f) * 2.0;
}


float reconstructNormalAxis(const vec2 N)
{
	return sqrt(1.0f - dot(N.xy, N.xy));
}
