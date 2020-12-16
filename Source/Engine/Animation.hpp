#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <assimp/scene.h>

#include <string>
#include <map>
#include <vector>

#include <Engine/GeomUtils.h>

class StaticMesh;

class SkeletalAnimation
{
public:
    SkeletalAnimation(const StaticMesh &mesh, const aiAnimation*, const aiScene*);
    ~SkeletalAnimation() = default;

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
        void insert(const double tick, const float4x4 trans)
        {
            if(std::find_if(mTick.begin(), mTick.end(), [tick](const Tick& t)
            {
                return tick == t.mTick;
            }) != mTick.end())
                return;

            mTick.insert(std::find_if(mTick.begin(), mTick.end(), [tick](const Tick& t)
            {
                return t.mTick > tick;
            }), Tick{tick, trans});
        }
        const std::vector<Tick>& getTicks() const
        {
            return mTick;
        }

    private:

        std::vector<Tick> mTick;
    };

    const std::string& getName() const
    {
        return mName;
    }

private:

    float4x4 interpolateTick(const Tick& lhs, const Tick& rhs, const double tick) const;

    void readNodeHierarchy(const aiAnimation* anim, const aiString &name, const aiNode* rootNode);
    float4x4 getParentTransform(const aiAnimation* anim, const aiNode* parent, const double tick);

    const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName);

    float4x4 interpolateScale(double time, const aiNodeAnim* pNodeAnim);
    float4x4 interpolateTranslation(double time, const aiNodeAnim* pNodeAnim);
    float4x4 interpolateRotation(double time, const aiNodeAnim* pNodeAnim);

    std::string mName;
    double mNumTicks;
    double mTicksPerSec;
    float4x4 mInverseGlobalTransform;

    std::map<std::string, BoneTransform> mBones;
};


struct MeshBlend
{
    MeshBlend(const aiAnimMesh*);

    std::vector<float3> mPosition;
    std::vector<float4> mNormals;
    std::vector<float4> mTangents;
    std::vector<float2> mUV;
    std::vector<uint32_t> mColours;

    std::string mName;
    float mWeight;
};


class BlendMeshAnimation
{
public:
    BlendMeshAnimation(const aiAnimation*, const aiScene*);
    ~BlendMeshAnimation() = default;

    double getTicksPerSec() const
    {
        return mTicksPerSecond;
    }

    double getTotalTicks() const
    {
        return mNumTicks;
    }

    std::vector<unsigned char> getBlendedVerticies(const StaticMesh&, const double tick) const;

    const std::string& getName() const
    {
        return mName;
    }

private:

    struct Tick
    {
        double mTime;
        
        std::vector<uint32_t> mVertexIndex;
        std::vector<double> mWeight;
    };

    std::string mName;
    double mTicksPerSecond;
    double mNumTicks;
    std::vector<Tick> mTicks;
};

#endif
