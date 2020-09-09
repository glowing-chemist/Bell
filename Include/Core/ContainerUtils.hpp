#ifndef CONTAINER_UTILS_HPP
#define CONTAINER_UTILS_HPP

#include <cstdint>

#include "BellLogging.hpp"


template<typename T, uint32_t N>
class StaticGrowableBuffer
{
public:
    StaticGrowableBuffer() :
    mSize{0} {}

    uint32_t size() const
    {
	return mSize;
    }

    void push_back(const T& val)
    {
	BELL_ASSERT(mSize < N, "Need to increase buffer size")
	mArray[mSize++] = val;
    }

    void clear()
    {
        mSize = 0;
    }

    T* begin()
    {
        return &mArray[0];
    }

    const T* begin() const
    {
        return &mArray[0];
    }

    T* end()
    {
        return &mArray[mSize];
    }

    const T* end() const
    {
        return &mArray[mSize];
    }

private:
    T mArray[N];
    uint32_t mSize;
};

#endif
