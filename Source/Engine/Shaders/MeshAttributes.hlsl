

#define  kAlphaTested 1
#define  kTransparent (1 << 1)

struct MeshEntry
{
    mat4 mTransformation;
    mat4 mPreviousTransformation;
    uint mFlags;
};
