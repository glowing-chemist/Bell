#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <assimp/scene.h>

#include <string>
#include <map>
#include <vector>

#include <Engine/GeomUtils.h>


class Animation
{
public:
    Animation(const aiAnimation*);
    ~Animation() = default;

    void tick() // advance the animation ticks.
    {
        ++mCurrentTick;
    }

    void reset()
    {
        mCurrentTick = 0;
    }

    uint32_t getTicksPerSec() const
    {
        return mTicksPerSec;
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
    std::string mName;
    uint32_t mNumTicks;
    uint32_t mTicksPerSec;
    uint32_t mCurrentTick;

    std::map<std::string, BoneTransform> mBones;
};

#endif
