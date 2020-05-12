#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <assimp/scene.h>

#include <string>
#include <map>
#include <vector>

#include <Engine/GeomUtils.h>

class StaticMesh;

class Animation
{
public:
    Animation(const StaticMesh &mesh, const aiAnimation*, const aiScene*);
    ~Animation() = default;

    std::vector<float4x4> calculateBoneMatracies(const StaticMesh&, const double tick);

    double getTicksPerSec() const
    {
        return mTicksPerSec;
    }

    double getTotalTicks() const
    {
        return mNumTicks;
    }

    struct Tick
    {
        double mTick;
        float4x4 mBoneTransform;
    };

    struct BoneTransform
    {
        std::vector<Tick> mTick;
    };

    const std::string& getName() const
    {
        return mName;
    }

private:

    float4x4 interpolateTick(const Tick& lhs, const Tick& rhs, const double tick) const;

    void readNodeHierarchy(const aiAnimation* pAnimation, const uint32_t keyFrameIndex, const aiNode* pNode, const float4x4& ParentTransform);
    const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName);

    std::string mName;
    double mNumTicks;
    double mTicksPerSec;
    float4x4 mInverseGlobalTransform;

    std::map<std::string, BoneTransform> mBones;
};

#endif
