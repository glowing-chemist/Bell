
	const float2 size = textureSize(sampler2D(inImage, linearSampler), 0);

	const int3 dispatchLocation = int3(gl_GlobalInvocationID);

	sharedLine[gl_LocalInvocationID.x + 3] = texture(sampler2D(inImage, linearSampler), float2(dispatchLocation.xy) / size);

	// Load the results either side as well.
	if(gl_LocalInvocationID.x < 3)
	{
		sharedLine[gl_LocalInvocationID.x] = texture(sampler2D(inImage, linearSampler), float2(dispatchLocation.xy - int2(3 - gl_LocalInvocationID.x, 0)) / size);
	}

	if(gl_LocalInvocationID.x == (KERNEL_SIZE - 1))
	{
		sharedLine[KERNEL_SIZE + 3] = texture(sampler2D(inImage, linearSampler), float2(dispatchLocation.xy + int2(1,0)) / size);
		sharedLine[KERNEL_SIZE + 4] = texture(sampler2D(inImage, linearSampler), float2(dispatchLocation.xy + int2(2,0)) / size);
		sharedLine[KERNEL_SIZE + 5] = texture(sampler2D(inImage, linearSampler), float2(dispatchLocation.xy + int2(3,0)) / size);
	}

	// wait for the shared memory to be fully populated.
	memoryBarrierShared();
	barrier();

	if(dispatchLocation.x >= size.x)
		return;

	float4 bluredPixel = blur(gl_LocalInvocationID.x);

	imageStore(outImage, dispatchLocation.xy, bluredPixel);