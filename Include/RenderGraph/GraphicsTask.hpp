#ifndef GRAPHICSTASK_HPP
#define GRAPHICSTASK_HPP

#include "RenderTask.hpp"

#include <string>
#include <map>
#include <vector>
#include <tuple>
#include <cstdint>


enum class DrawType
{
    Standard,
    Indexed,
    Instanced,
    Indirect,
    IndexedInstanced,
    IndexedInstancedIndirect
};

enum class BlendMode
{
    None,
    Add,
    Subtract,
    Min,
    Max
};

struct Rect
{
    uint32_t x, y;
};

struct RenderPipelineDescription
{
    std::string mVertexShaderName;
    std::string mGeometryShaderName;
    std::string mHullShaderName;
    std::string mTesselationControlShaderName;
    std::string mFragmentShaderName;

    Rect mScissorRect;
    Rect mViewport;

    BlendMode mBlendMode;
};


// This class describes a renderpass with all of it's inputs and
// outputs.
class GraphicsTask : public RenderTask
{
public:
	GraphicsTask(const std::string& name, const RenderPipelineDescription& desc) : mName{ name }, mPipelineDescription{ desc } {}

	const std::string& getName() { return mName;  }

	void addDrawCall(const uint32_t vertexOffset, const uint32_t numberOfVerticies) 
	{ 
			mDrawCalls.push_back({DrawType::Standard, vertexOffset, numberOfVerticies, 0, 0, 0});
	}

	void addIndexedDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) 
	{ 
			mDrawCalls.push_back({ DrawType::Indexed, vertexOffset, 0, indexOffset, numberOfIndicies, 0 });
	}

	void addIndexedInstancedDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
	{
		mDrawCalls.push_back({ DrawType::IndexedInstanced, vertexOffset, 0, indexOffset, numberOfIndicies, numberOfInstances });
	}

	void addIndexedInstancedIndirectDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
	{
		mDrawCalls.push_back({ DrawType::IndexedInstanced, vertexOffset, 0, indexOffset, numberOfIndicies, numberOfInstances });
	}

	void clearCalls() override { mDrawCalls.clear(); }

private:
	std::string mName;

	struct thunkedDraw {
		DrawType mDrawType;
		uint32_t mVertexOffset;
		uint32_t mNumberOfVerticies;
		uint32_t mIndexOffset;
		uint32_t mNumberOfIndicies;
		uint32_t mNumberOfInstances;
	};
	std::vector<thunkedDraw> mDrawCalls;

	RenderPipelineDescription mPipelineDescription;
};

#endif
