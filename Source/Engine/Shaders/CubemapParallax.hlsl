// Cubemap parallax sampling


vec3 ParallaxCubemapNormal(const vec3 centre, const float cubeDimension, const vec3 worldPosition, const vec3 normal)
{
	// See Bell notebook for details
	return normalize((normal + worldPosition) - centre);
}
