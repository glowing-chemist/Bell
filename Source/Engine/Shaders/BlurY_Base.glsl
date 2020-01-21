
	const vec2 size = textureSize(sampler2D(inImage, linearSampler), 0);

	const ivec3 dispatchLocation = ivec3(gl_GlobalInvocationID);

	sharedLine[gl_LocalInvocationID.y + 3] = texture(sampler2D(inImage, linearSampler), vec2(dispatchLocation.xy) / size);

	// Load the results either side as well.
	if(gl_LocalInvocationID.y < 3)
	{
		sharedLine[gl_LocalInvocationID.y] = texture(sampler2D(inImage, linearSampler), vec2(dispatchLocation.xy - ivec2(0, 3 - gl_LocalInvocationID.y)) / size);
	}

	if(gl_LocalInvocationID.y == (KERNAL_SIZE - 1))
	{
		sharedLine[KERNAL_SIZE + 3] = texture(sampler2D(inImage, linearSampler), vec2(dispatchLocation.xy + ivec2(0,1)) / size);
		sharedLine[KERNAL_SIZE + 4] = texture(sampler2D(inImage, linearSampler), vec2(dispatchLocation.xy + ivec2(0,2)) / size);
		sharedLine[KERNAL_SIZE + 5] = texture(sampler2D(inImage, linearSampler), vec2(dispatchLocation.xy + ivec2(0,3)) / size);
	}

	// wait for the shared memory to be fully populated.
	groupMemoryBarrier();
	barrier();

	if(dispatchLocation.y >= size.y)
		return;

	vec4 bluredPixel = blur(gl_LocalInvocationID.y);

	imageStore(outImage, dispatchLocation.xy, bluredPixel);