// Cubemap parallax sampling


float3 ParallaxCubemapNormal(const float3 centre, const float cubeDimension, const float3 worldPosition, const float3 normal)
{
	// See Bell notebook for details
	return normalize((normal + worldPosition) - centre);
}
