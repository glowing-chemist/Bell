

// Utility functions for normal mapping
// view vector is camera.pos - fragment.pos;
mat3 tangentSpaceMatrix(const vec3 vertNormal, const vec3 view, const vec4 uvDerivitives)
{
	// we don't/can't compute these here as we're doing defered textureing
	const vec2 uvDx = uvDerivitives.xy;
	const vec3 uvDy = uvDerivitives.zw;

	const vec3 viewDx = dFdx(view);
	const vec3 viewDy = dFdy(view);

	 // solve the linear system 
	 vec3 dp2perp = cross(viewDy, vertNormal);
	 vec3 dp1perp = cross(vertNormal, viewDx);
	 vec3 tangent = dp2perp * uvDx.x + dp1perp * uvDy.x;
	 vec3 bitangent = dp2perp * uvDx.y + dp1perp * uvDy.y;
	 
	 // construct a scale-invariant frame
	 float invmax = inversesqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));

	 return mat3(tangent * invmax, bitangent * invmax, vertNormal);
}