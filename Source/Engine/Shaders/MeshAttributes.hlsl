

#define  kAlphaTested 1
#define  kTransparent (1 << 1)

struct MeshEntry
{
    float4x4 mTransformation;
    float4x4 mPreviousTransformation;
    uint mFlags;
};
