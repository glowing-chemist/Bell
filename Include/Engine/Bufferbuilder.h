#ifndef BUFFERBUILDER_HPP
#define BUFFERBUILDER_HPP

#include <cstdint>
#include <vector>

class BufferBuilder
{
public:
	BufferBuilder() = default;

	template<typename T>
    uint64_t addData(const std::vector<T>&);
	uint64_t addData(const void*, const size_t);

	const std::vector<unsigned char>& finishRecording() const
	{
		return mBytes;
	}

    void reset()
    {
        mBytes.clear();
    }

private:

	std::vector<unsigned char> mBytes;

};


template<typename T>
inline uint64_t BufferBuilder::addData(const std::vector<T>& data)
{
    const uint64_t offset = mBytes.size();

	const uint64_t dataSize = data.size() * sizeof(T);
    mBytes.insert(mBytes.end(), reinterpret_cast<const unsigned char*>(data.data()), reinterpret_cast<const unsigned char*>(data.data()) + dataSize);

    return offset;
}


inline uint64_t BufferBuilder::addData(const void* ptr, const size_t size)
{
	const uint64_t offset = mBytes.size();

	mBytes.insert(mBytes.end(), reinterpret_cast<const unsigned char*>(ptr), reinterpret_cast<const unsigned char*>(ptr) + size);

	return offset;
}

#endif

