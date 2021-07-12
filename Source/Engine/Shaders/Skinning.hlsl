
#define ShadeFlag_Skinning (1 << 22)

struct BoneIndex
{
    uint mBone;
    float mWeight;
};

struct Bone
{
	float4x4 transform;
};

float4x4 calculateSkinningTransform(const uint vertexIndex, const uint boneOffset, StructuredBuffer<uint2> bonesPerVertex, StructuredBuffer<uint2> boneIndexAndWeights, StructuredBuffer<float4x4> bones)
{
	uint2 vertexBoneOffsetAndCount = bonesPerVertex[vertexIndex];

	float4x4 transform = float4x4(	float4(0.0f, 0.0f, 0.0f, 0.0f), 
									float4(0.0f, 0.0f, 0.0f, 0.0f), 
									float4(0.0f, 0.0f, 0.0f, 0.0f), 
									float4(0.0f, 0.0f, 0.0f, 0.0f));
	for(uint i = 0; i < vertexBoneOffsetAndCount.y; ++i)
	{
		uint2 integerIndexAndWeight = boneIndexAndWeights[vertexBoneOffsetAndCount.x + i];
		BoneIndex index;
		index.mBone = integerIndexAndWeight.x;
		index.mWeight = asfloat(integerIndexAndWeight.y);
		const float4x4 boneTransform = bones[boneOffset + index.mBone];

		transform += boneTransform * index.mWeight;
	}

	return transform;
}