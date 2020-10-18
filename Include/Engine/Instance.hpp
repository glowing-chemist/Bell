#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include "Engine/GeomUtils.h"

#include <string>


class Instance
{
public:
    Instance(const float4x4& trans, const std::string& name) :
        mParent(nullptr),
        mTransformation(trans),
        mPreviousTransformation(trans),
        mName{name} {}

    virtual ~Instance() = default;

    void setParentInstance(const Instance* parent)
    {
        mParent = parent;
    }

    float4x4 getTransMatrix() const
    {
        if(mParent)
        {
            return mTransformation * mParent->getTransMatrix();
        }
        else
            return mTransformation;
    }

    void setTransMatrix(const float4x4& newTrans)
    {
        mPreviousTransformation = mTransformation;
        mTransformation = newTrans;
    }

    const std::string& getName() const
    {
        return mName;
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

    const Instance* mParent;
    float4x4 mTransformation;
    float4x4 mPreviousTransformation;
    std::string mName;
};

#endif
