{
	uint xSize, ySize;
	inImage.GetDimensions(xSize, ySize);

	const float2 size = float2(xSize, ySize);

	const int3 dispatchLocation = int3(globalIndex);

	sharedLine[localIndex.y + 3] = inImage.Sample(linearSampler, float2(dispatchLocation.xy) / size);

	// Load the results either side as well.
	if(localIndex.y < 3)
	{
		sharedLine[localIndex.y] = inImage.Sample(linearSampler, float2(dispatchLocation.xy - int2(0, 3 - localIndex.y)) / size);
	}

	if(localIndex.y == (KERNEL_SIZE - 1))
	{
		sharedLine[KERNEL_SIZE + 3] = inImage.Sample(linearSampler, float2(dispatchLocation.xy + int2(0,1)) / size);
		sharedLine[KERNEL_SIZE + 4] = inImage.Sample(linearSampler, float2(dispatchLocation.xy + int2(0,2)) / size);
		sharedLine[KERNEL_SIZE + 5] = inImage.Sample(linearSampler, float2(dispatchLocation.xy + int2(0,3)) / size);
	}

	// wait for the shared memory to be fully populated.
	AllMemoryBarrierWithGroupSync();

	if(dispatchLocation.y >= size.y)
		return;

	outImage[dispatchLocation.xy] = blur(localIndex.y);
}