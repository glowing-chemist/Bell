#define KERNEL_SIZE 256


const float[] decomposedKernel =  {0.030078323, 0.104983664, 0.222250419, 0.285375187, 0.222250419, 0.104983664, 0.030078323};


// extra for eaxtr samples required at end.
shared vec4 sharedLine[KERNEL_SIZE + (2 * 3)];


vec4 blur(uint position)
{
	vec4 bluredPixel = vec4(0.0f, 0.0f, 0.0f, 0.0f);

	for(uint i = 0; i < 7; ++i)
	{
		bluredPixel += decomposedKernel[i] * sharedLine[position + i];
	}

	return bluredPixel;
}