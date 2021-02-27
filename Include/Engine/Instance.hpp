#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include "Engine/GeomUtils.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <string>


class Instance
{
public:
    Instance(const float4x4& trans, const std::string& name) :
        mParent(nullptr),
        mPosition(0.0f, 0.0f, 0.f),
        mRotation(1.0f, 0.0f, 0.0f, 0.0f),
        mScale(10.f, 1.0f ,1.0f),
        mPreviousTransformation(trans),
        mName{name}
        {
            float3 skew;
            float4 persp;
            glm::decompose(trans, mScale, mRotation, mPosition, skew, persp);
        }

    Instance(const float3 position, const quat& rotation, const float3& scale, const std::string& name) :
            mParent(nullptr),
            mPosition(position),
            mRotation(rotation),
            mScale(scale),
            mPreviousTransformation(createTransMatrix()),
            mName{name}
    {
    }

    virtual ~Instance() = default;

    void setParentInstance(const Instance* parent)
    {
        mParent = parent;
    }

    const Instance* getParent() const
    {
        return mParent;
    }

    float4x4 getTransMatrix() const
    {
        if(mParent)
        {
            return createTransMatrix() * mParent->getTransMatrix();
        }
        else
            return createTransMatrix();
    }

    void setPosition(const float3& pos)
    {
        mPosition = pos;
    }

    const float3& getPosition() const
    {
        return mPosition;
    }

    void setRotation(const quat& rot)
    {
        mRotation = rot;
    }

    const quat& getRotation() const
    {
        return mRotation;
    }

    void setScale(const float3& scale)
    {
        mScale = scale;
    }

    const float3& getScale() const
    {
        return mScale;
    }

    const std::string& getName() const
    {
        return mName;
    }

    void newFrame()
    {
        mPreviousTransformation = createTransMatrix();
    }

protected:

    float4x4 getPreviousTransMatrix() const
    {
        if(mParent)
        {
            return mPreviousTransformation * mParent->getPreviousTransMatrix();
        }
        else
            return mPreviousTransformation;
    }

    float4x4 createTransMatrix() const
    {
        return glm::translate(mPosition) * glm::mat4_cast(mRotation) * glm::scale(mScale);
    }


    const Instance* mParent;
    float3 mPosition;
    quat mRotation;
    float3 mScale;
    float4x4 mPreviousTransformation;
    std::string mName;
};

#endif
