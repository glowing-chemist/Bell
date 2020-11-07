

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

float3x3 tangentSpaceMatrix(float3 normal, float4 tangent)
{
	const float3 bitangent = normalize(cross(normal, tangent.xyz)) * tangent.w;

	return float3x3(tangent.xyz, bitangent, normal);
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

float2 OctWrap( float2 v )
{
    return ( 1.0 - abs( v.yx ) ) * ( v.xy >= 0.0 ? 1.0 : -1.0 );
}
 
float2 encodeOct( float3 n )
{
    n /= ( abs( n.x ) + abs( n.y ) + abs( n.z ) );
    n.xy = n.z >= 0.0 ? n.xy : OctWrap( n.xy );
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}
 
float3 decodeOct( float2 f )
{
    f = f * 2.0 - 1.0;
 
    float3 n = float3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
    float t = saturate( -n.z );
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize( n );
}
