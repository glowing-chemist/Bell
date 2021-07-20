
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

float4x4 calculateSkinningTransform(const SkinnedVertex vertex, const uint boneOffset, StructuredBuffer<float4x4> bones)
{
	float4x4 transform = float4x4(	float4(0.0f, 0.0f, 0.0f, 0.0f), 
									float4(0.0f, 0.0f, 0.0f, 0.0f), 
									float4(0.0f, 0.0f, 0.0f, 0.0f), 
									float4(0.0f, 0.0f, 0.0f, 0.0f));
	for(uint i = 0; i < 4; ++i)
	{
		const float4x4 boneTransform = bones[boneOffset + vertex.boneIndicies[i]];

		transform += boneTransform * vertex.boneWeights[i];
	}

	return transform;
}