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

    struct BoneTransform
    {
        BoneTransform() :
            mPositions{},
            mScales{},
            mRotations{} {}

        float4x4 getBoneTransform(const double tick);

        struct PositionKey
        {
            double mTime;
            float3 mValue;
        };

        struct ScaleKey
        {
            double mTime;
            float3 mValue;
        };

        struct RotationKey
        {
            double mTime;
            quat mValue;
        };

        std::vector<PositionKey> mPositions;
        std::vector<ScaleKey> mScales;
        std::vector<RotationKey> mRotations;

    private:

        float3 interpolateScale(double time);
        float3 interpolateTranslation(double time);
        quat interpolateRotation(double time);

    };

    const std::string& getName() const
    {
        return mName;
    }

private:

    void readNodeHierarchy(const aiAnimation* anim, const std::string& name, const aiNode* rootNode);

    const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName);

    std::string mName;
    double mNumTicks;
    double mTicksPerSec;
    float4x4 mRootTransform;

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
