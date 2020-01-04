#ifndef GL_BUFFER_VIEW_HPP
#define GL_BUFFER_VIEW_HPP

#include "Core/Buffer.hpp"
#include "Core/BufferView.hpp"


class OpenGLBufferView : public BufferViewBase
{
public:
	OpenGLBufferView(Buffer&, const uint64_t offset = 0, const uint64_t size = ~0ull);
	~OpenGLBufferView() = default;


	uint32_t getBuffer() const
	{
		return mBufferHandle;
	}


private:

	uint32_t mBufferHandle;

};


#endif