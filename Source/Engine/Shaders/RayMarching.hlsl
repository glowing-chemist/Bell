

float2 marchRay(float3 position, const float3 direction, const uint maxSteps, float2 pixelSize, uint startLOD, const float near, const float far, float4x4 invProj)
{
	uint currentLOD = startLOD;
	float3 currentPosition = position.xyz;
	float3 maxPizelSize = max(pixelSize.x, pixelSize.y);

	for(uint i = 0; i < maxSteps; ++i)
	{
		currentPosition += direction * maxPizelSize;

		float2 uv = (currentPosition.xy * 0.5f) + 0.5f;
		const float steppedDepth = (LinearDepth.SampleLevel(linearSampler, uv, currentLOD).x * far) + near;

		// No need for a full matrix mul here.
		float cameraSpaceDepth = dot(invProj[2], float4(currentPosition, 1.0f));
		const float cameraW = dot(invProj[3], float4(currentPosition, 1.0f));
		cameraSpaceDepth = -(cameraSpaceDepth / cameraW);

		if(cameraSpaceDepth > steppedDepth && currentLOD == 0)
			return uv;
		else if(cameraSpaceDepth >= steppedDepth)
		{
			--currentLOD;
			maxPizelSize /= 2.0f;
			currentPosition -= direction * maxPizelSize;
		}
	}

	return float2(-1.0f, -1.0f);;
}