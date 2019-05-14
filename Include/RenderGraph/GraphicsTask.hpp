#ifndef GRAPHICSTASK_HPP
#define GRAPHICSTASK_HPP

#include "RenderTask.hpp"
#include "Core/Shader.hpp"

#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <tuple>
#include <utility>
#include <cstdint>


enum class DrawType
{
    Standard,
    Indexed,
    Instanced,
    Indirect,
    IndexedInstanced,
    IndexedIndirect
};

enum class BlendMode
{
    None,
	Add,
	Subtract,
	ReverseSubtract,
	Min,
	Max
};

enum class DepthTest
{
    None,
    Less,
    LessEqual,
    Equal,
    GreaterEqual,
    Greater
};

enum class VertexAssemblyType
{
	vec1 = 1,
	vec2 = 2,
	vec3 = 3,
	vec4 = 4,
	ivec1 = 1,
	ivec2 = 2,
	ivec3 = 3,
	ivec4 = 4,
	f = 1,
	i = 1
};

enum VertexAttributes
{
	Position2 = 1 << 1,
	Position3 = 1 << 2,
	Position4 = 1 << 3,
	TextureCoordinates = 1 << 4,
	Normals = 1 << 5,
	Aledo = 1 << 6
};

struct Rect
{
    uint32_t x, y;
};

struct GraphicsPipelineDescription
{
    Shader mVertexShader;
    std::optional<Shader> mGeometryShader;
    std::optional<Shader> mHullShader;
    std::optional<Shader> mTesselationControlShader;
    Shader mFragmentShader;

    Rect mScissorRect;
    Rect mViewport;
	bool mUseBackFaceCulling;

	BlendMode mAlphaBlendMode;
	BlendMode mColourBlendMode;
    DepthTest mDepthTest;
};
// needed in order to use unordered_map
namespace std
{
	template<>
	struct hash<GraphicsPipelineDescription> 
	{
		size_t operator()(const GraphicsPipelineDescription&) const noexcept;
	};
}

// This class describes a renderpass with all of it's inputs and
// outputs.
class GraphicsTask : public RenderTask
{
public:
    GraphicsTask(const std::string& name, const GraphicsPipelineDescription& desc) : RenderTask{name}, mPipelineDescription{ desc } {}
	
	GraphicsPipelineDescription getPipelineDescription() const { return mPipelineDescription; }

	void setVertexAttributes(int vertexAttributes)
		{ mVertexAttributes = vertexAttributes; }

	int getVertexAttributes() const { return mVertexAttributes; }

	void addDrawCall(const uint32_t vertexOffset, const uint32_t numberOfVerticies) 
	{ 
            mDrawCalls.push_back({DrawType::Standard, vertexOffset, numberOfVerticies, 0, 0, 1, ""});
	}

	void addIndexedDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) 
	{ 
            mDrawCalls.push_back({ DrawType::Indexed, vertexOffset, 0, indexOffset, numberOfIndicies, 1, "" });
	}

    void addIndirectDrawCall(const uint32_t drawCalls, const std::string&& indirectBuffer)
    {
        mDrawCalls.push_back({DrawType::Indirect, 0, 0, 0, 0, drawCalls, indirectBuffer});
    }

	void addIndexedInstancedDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
	{
        mDrawCalls.push_back({ DrawType::IndexedInstanced, vertexOffset, 0, indexOffset, numberOfIndicies, numberOfInstances, "" });
	}

    void addIndexedIndirectDrawCall(const uint32_t drawCalls, const uint32_t indexOffset, const uint32_t numberOfIndicies, const std::string&& indirectName)
	{
        mDrawCalls.push_back({ DrawType::IndexedIndirect, 0, 0, indexOffset, numberOfIndicies, drawCalls, indirectName});
	}

    void recordCommands(vk::CommandBuffer, const RenderGraph&) const override;

    std::vector<vk::ClearValue> getClearValues() const;

    void mergeDraws(GraphicsTask&);

	void clearCalls() override { mDrawCalls.clear(); }
	TaskType taskType() const override { return TaskType::Graphics; }

    friend bool operator==(const GraphicsTask& lhs, const GraphicsTask& rhs);

private:

	struct thunkedDraw {
		DrawType mDrawType;
		uint32_t mVertexOffset;
		uint32_t mNumberOfVerticies;
		uint32_t mIndexOffset;
		uint32_t mNumberOfIndicies;
		uint32_t mNumberOfInstances;
        std::string mIndirectBufferName;
	};
	std::vector<thunkedDraw> mDrawCalls;

	GraphicsPipelineDescription mPipelineDescription;
	
	int mVertexAttributes;
};


bool operator==(const GraphicsPipelineDescription& lhs, const GraphicsPipelineDescription& rhs);

#endif
