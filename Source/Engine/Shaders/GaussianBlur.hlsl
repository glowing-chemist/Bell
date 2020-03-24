#define KERNEL_SIZE 256

// extra for eaxtr samples required at end.
groupshared float4 sharedLine[KERNEL_SIZE + (2 * 3)];


float4 blur(uint position)
{
	const float decomposedKernel[] =  {0.030078323, 0.104983664, 0.222250419, 0.285375187, 0.222250419, 0.104983664, 0.030078323};

	float4 bluredPixel = float4(0.0f, 0.0f, 0.0f, 0.0f);

	[unroll]
	for(uint i = 0; i < 7; ++i)
	{
		bluredPixel += decomposedKernel[i] * sharedLine[position + i];
	}

	return bluredPixel;
}