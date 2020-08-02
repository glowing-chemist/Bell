

float2 marchRay(float3 position, const float3 direction, const uint maxSteps, float2 pixelSize, uint startLOD)
{
	uint currentLOD = startLOD;
	const float2 startUV = ((position.xy * 0.5f) + 0.5f);
	float2 uv = startUV;
	const float2 xyDir = normalize(direction.xy); 

	for(uint i = 0; i < maxSteps; ++i)
	{
		uv += xyDir * pixelSize;

		const float steppedDepth = LinearDepth.SampleLevel(linearSampler, uv, currentLOD).x;
		const float rayDepth = position.z + abs(length(startUV - uv) / length(direction.xy)) * direction.z;

		if(rayDepth >= steppedDepth && currentLOD == 0)
			return uv;
		else if(rayDepth >= steppedDepth)
		{
			--currentLOD;
			pixelSize /= float2(2.0f, 2.0f);
			uv -= xyDir * pixelSize;
		}
	}

	return float2(-1.0f, -1.0f);
}