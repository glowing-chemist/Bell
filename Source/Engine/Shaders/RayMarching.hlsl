

float2 marchRay(float3 position, const float3 direction, const uint maxSteps, float2 pixelSize)
{
	uint currentLOD = 4;
	const float2 startUV = (position.xy * 0.5f + 0.5f);
	float2 uv = startUV;
	const float2 xyDir = normalize(direction.xy); 

	for(uint i = 0; i < maxSteps; ++i)
	{
		uv += xyDir * pixelSize;

		const float steppedDepth = LinearDepth.SampleLevel(linearSampler, uv, currentLOD);
		const float rayDepth = position.z + abs((startUV.x - uv.x) / direction.x) * direction.z;

		if(rayDepth >= steppedDepth && currentLOD == 0)
			return uv;
		else if(rayDepth >= steppedDepth)
		{
			--currentLOD;
			pixelSize /= float2(2.0f, 2.0f);
		}
		else if(currentLOD < 4)
		{
			++currentLOD;
			pixelSize *= float2(2.0f, 2.0f);
		}
	}

	return float2(-1.0f, -1.0f);
}