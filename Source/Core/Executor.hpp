#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include "glm/mat4x4.hpp"

class Buffer;
class BufferView;


class Executor
{
public:
	Executor() = default;
	virtual ~Executor() = default;

	virtual void draw(const uint32_t vertexOffset, const uint32_t vertexCount) = 0;

	virtual void instancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount) = 0;

	virtual void indexedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) = 0;

	virtual void indexedInstancedDraw(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies) = 0;

	virtual void indirectDraw(const uint32_t drawCalls, const const BufferView&) = 0;

	virtual void indexedIndirectDraw(const uint32_t drawCalls, const BufferView&) = 0;

	virtual void insertPushConsatnt(const glm::mat4& val) = 0;

	virtual void dispatch(const uint32_t x, const uint32_t y, const uint32_t z) = 0;

	virtual void dispatchIndirect(const BufferView&) = 0;

	virtual void bindVertexBuffer(const Buffer&, const size_t offset) = 0;
};

#endif