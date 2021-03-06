#ifndef BUFFERVIEW_HPP
#define BUFFERVIEW_HPP

#include "Buffer.hpp"

#include <limits>
#include <memory>

class BarrierRecorder;

class BufferViewBase : public DeviceChild
{
    friend BarrierRecorder;
public:

	BufferViewBase(Buffer&, const uint64_t offset = 0, const uint64_t size = ~0ull);
    // buffer views don't manage any resources so can have a trivial destructor.
    virtual ~BufferViewBase() = default;

    uint64_t getOffset() const
    { return mOffset; }

    uint64_t getSize() const
    { return mSize; }

    BufferUsage getUsage() const
    { return mUsage; }


private:

    uint64_t mOffset;
    uint64_t mSize;

    BufferUsage mUsage;
};


class BufferView
{
public:

    BufferView(Buffer&, const uint64_t offset = 0, const uint64_t size = ~0ull);
	~BufferView() = default;


	BufferViewBase* getBase()
	{
		return mBase.get();
	}

	const BufferViewBase* getBase() const
	{
		return mBase.get();
	}

	BufferViewBase* operator->()
	{
		return getBase();
	}

	const BufferViewBase* operator->() const
	{
		return getBase();
	}

private:

	std::shared_ptr<BufferViewBase> mBase;

};

#endif
