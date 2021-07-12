#define KERNEL_SIZE 256

// extra for eaxtr samples required at end.
groupshared VEC_TYPE sharedLine[KERNEL_SIZE + (2 * 3)];


VEC_TYPE blur(uint position)
{
	const float decomposedKernel[] =  {0.030078323, 0.104983664, 0.222250419, 0.285375187, 0.222250419, 0.104983664, 0.030078323};

	VEC_TYPE bluredPixel = 0.0f;

	[unroll(7)]
	for(uint i = 0; i < 7; ++i)
	{
		bluredPixel += decomposedKernel[i] * sharedLine[position + i];
	}

	return bluredPixel;
}