
struct KdNode
{
    float4 pivot;
    uint   pivotIndex;
    int    leftIndex;
    int    rightIndex;
    uint _padding;
};


uint findNearestPoint(StructuredBuffer<KdNode> tree, const float3 p)
{
	bool found = false;
	uint clostestIndex = 0;

	uint currentDepth = 0;
	uint nextNodeIndex = 0;

	KdNode root = tree[0];

	while(!found)
	{
		const uint axis = currentDepth % 3; // alternate x -> y -> z -> x ->...

		float leftDistance = 1.#INF;
		KdNode leftNode;
		if(root.leftIndex != -1)
		{
			leftNode = tree[root.leftIndex];
			leftDistance = abs(p[axis] - leftNode.pivot[axis]); 
		}

		float rightDistance = 1.#INF;
		KdNode rightNode;
		if(root.rightIndex != -1)
		{
			rightNode = tree[root.rightIndex];
			rightDistance = abs(p[axis] - rightNode.pivot[axis]); 
		}

		if(leftDistance < rightDistance)
		{
			root = leftNode;
		}
		else if(rightDistance < leftDistance)
		{
			root = rightNode;
		}
		else // we are at a leaf.
		{
			clostestIndex = root.pivotIndex;
			found = true;
		}

		++currentDepth;
	}

	return clostestIndex;
}