{
	uint xSize, ySize;
	inImage.GetDimensions(xSize, ySize);

	const float2 size = float2(xSize, ySize);

	const int3 dispatchLocation = int3(globalIndex);

	sharedLine[localIndex.x + 3] = inImage.Sample(linearSampler, float2(dispatchLocation.xy) / size);

	// Load the results either side as well.
	if(localIndex.x < 3)
	{
		sharedLine[localIndex.x] = inImage.Sample(linearSampler, float2(dispatchLocation.xy - int2(3 - localIndex.x, 0)) / size);
	}

	if(localIndex.x == (KERNEL_SIZE - 1))
	{
		sharedLine[KERNEL_SIZE + 3] = inImage.Sample(linearSampler, float2(dispatchLocation.xy + int2(1,0)) / size);
		sharedLine[KERNEL_SIZE + 4] = inImage.Sample(linearSampler, float2(dispatchLocation.xy + int2(2,0)) / size);
		sharedLine[KERNEL_SIZE + 5] = inImage.Sample(linearSampler, float2(dispatchLocation.xy + int2(3,0)) / size);
	}

	// wait for the shared memory to be fully populated.
	AllMemoryBarrierWithGroupSync();

	if(dispatchLocation.x >= size.x)
		return;

	outImage[dispatchLocation.xy] = blur(localIndex.x);
}