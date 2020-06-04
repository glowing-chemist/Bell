{
	uint xSize, ySize;
	inImage.GetDimensions(xSize, ySize);

	const int3 dispatchLocation = int3(globalIndex);

	if(dispatchLocation.x >= xSize)
		return;

	const float2 size = float2(xSize, ySize);

	sharedLine[localIndex.x + 3] = inImage.Load(int3(dispatchLocation.xy, 0));

	// Load the results either side as well.
	if(localIndex.x < 3)
	{
		sharedLine[localIndex.x] = inImage.Load(int3(dispatchLocation.xy - int2(3 - localIndex.x, 0), 0));
	}

	if(localIndex.x == (KERNEL_SIZE - 1))
	{
		sharedLine[KERNEL_SIZE + 3] = inImage.Load(int3(dispatchLocation.xy + int2(1,0), 0));
		sharedLine[KERNEL_SIZE + 4] = inImage.Load(int3(dispatchLocation.xy + int2(2,0), 0));
		sharedLine[KERNEL_SIZE + 5] = inImage.Load(int3(dispatchLocation.xy + int2(3,0), 0));
	}

	// wait for the shared memory to be fully populated.
	AllMemoryBarrierWithGroupSync();

	outImage[dispatchLocation.xy] = blur(localIndex.x);
}