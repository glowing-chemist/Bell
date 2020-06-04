{
	uint xSize, ySize;
	inImage.GetDimensions(xSize, ySize);

	const int3 dispatchLocation = int3(globalIndex);

	if(dispatchLocation.y >= ySize)
		return;

	sharedLine[localIndex.y + 3] = inImage.Load(int3(dispatchLocation.xy, 0));

	// Load the results either side as well.
	if(localIndex.y < 3)
	{
		sharedLine[localIndex.y] = inImage.Load(int3(dispatchLocation.xy - int2(0, 3 - localIndex.y), 0));
	}

	if(localIndex.y == (KERNEL_SIZE - 1))
	{
		sharedLine[KERNEL_SIZE + 3] = inImage.Load(int3(dispatchLocation.xy + int2(0,1), 0));
		sharedLine[KERNEL_SIZE + 4] = inImage.Load(int3(dispatchLocation.xy + int2(0,2), 0));
		sharedLine[KERNEL_SIZE + 5] = inImage.Load(int3(dispatchLocation.xy + int2(0,3), 0));
	}

	// wait for the shared memory to be fully populated.
	AllMemoryBarrierWithGroupSync();

	outImage[dispatchLocation.xy] = blur(localIndex.y);
}