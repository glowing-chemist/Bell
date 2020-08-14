

float2 marchRay(float3 position, const float3 direction, const uint maxSteps, float2 pixelSize, uint startLOD)
{
	uint currentLOD = startLOD;
	float3 currentPosition = position.xyz;
	float maxPizelSize = max(pixelSize.x, pixelSize.y);

	for(uint i = 0; i < maxSteps; ++i)
	{
		currentPosition += direction * maxPizelSize;

		float2 uv = (currentPosition.xy * 0.5f) + 0.5f;
		uv.y = 1.0f - uv.y;
		const float steppedDepth = LinearDepth.SampleLevel(linearSampler, uv, currentLOD).x;

		if(currentPosition.z > steppedDepth && currentLOD == 0)
			return uv;
		else if(currentPosition.z >= steppedDepth)
		{
			--currentLOD;
			maxPizelSize /= 2.0f;
			currentPosition -= direction * maxPizelSize;
		}
	}

	return float2(-1.0f, -1.0f);
}