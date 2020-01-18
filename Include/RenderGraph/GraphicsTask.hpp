#ifndef GRAPHICSTASK_HPP
#define GRAPHICSTASK_HPP

#include "RenderTask.hpp"
#include "Core/Shader.hpp"
#include "Engine/GeomUtils.h"

#include <glm/mat4x4.hpp>

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
	IndexedIndirect,
	SetPushConstant
};

enum VertexAttributes
{
	Position2 = 1 << 1,
	Position3 = 1 << 2,
	Position4 = 1 << 3,
	TextureCoordinates = 1 << 4,
	Normals = 1 << 5,
    Albedo = 1 << 6,
	Material = 1 << 7
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

enum class Primitive
{
	TriangleList,
	TriangleStrip,
	LineStrip,
	LineList,
	Point
};


struct ClearValues
{
    ClearValues(const float red, const float green, const float blue, const float alpha) :
        r{red}, g{green}, b{blue}, a{alpha} {}

    float r, g, b, a;
};


struct GraphicsPipelineDescription
{
	Shader mVertexShader;
	std::optional<Shader> mInstancedVertexShader;
	std::optional<Shader> mGeometryShader;
	std::optional<Shader> mHullShader;
	std::optional<Shader> mTesselationControlShader;
	Shader mFragmentShader;

	Rect mScissorRect;
	Rect mViewport;
	bool mUseBackFaceCulling;

	BlendMode mAlphaBlendMode;
	BlendMode mColourBlendMode;
	bool      mDepthWrite;
	DepthTest mDepthTest;

	Primitive mPrimitiveType;

	GraphicsPipelineDescription(const Shader& vert, const Shader& frag,
		const Rect& scissor, const Rect& viewport) :
		mVertexShader{ vert },
		mInstancedVertexShader{},
		mGeometryShader{},
		mHullShader{},
		mTesselationControlShader{},
		mFragmentShader{ frag },
		mScissorRect{ scissor },
		mViewport{ viewport },
		mUseBackFaceCulling{ true },
		mAlphaBlendMode{ BlendMode::None },
		mColourBlendMode{ BlendMode::None },
		mDepthWrite{ false },
		mDepthTest{ DepthTest::None },
		mPrimitiveType{ Primitive::TriangleList }
	{}

	GraphicsPipelineDescription(const Shader& vert, const Shader& frag,
		const Rect& scissor, const Rect& viewport,
		const bool useFaceCulling,
		const BlendMode alphaBlendMode,
		const BlendMode colourBlendMode,
		const bool depthWrite,
		const DepthTest depthTest,
		const Primitive primitiveType) :
		mVertexShader{ vert },
		mInstancedVertexShader{},
		mGeometryShader{},
		mHullShader{},
		mTesselationControlShader{},
		mFragmentShader{ frag },
		mScissorRect{ scissor },
		mViewport{ viewport },
		mUseBackFaceCulling{ useFaceCulling },
		mAlphaBlendMode{ alphaBlendMode },
		mColourBlendMode{ colourBlendMode },
		mDepthWrite{ depthWrite },
		mDepthTest{ depthTest },
		mPrimitiveType{ primitiveType }
	{}

	GraphicsPipelineDescription(const Shader& vert,
		const Shader& instancedVert,
		const Shader& frag,
		const Rect& scissor, const Rect& viewport,
		const bool useFaceCulling,
		const BlendMode alphaBlendMode,
		const BlendMode colourBlendMode,
		const bool depthWrite,
		const DepthTest depthTest,
		const Primitive primitiveType) :
		mVertexShader{ vert },
		mInstancedVertexShader{ instancedVert },
		mGeometryShader{},
		mHullShader{},
		mTesselationControlShader{},
		mFragmentShader{ frag },
		mScissorRect{ scissor },
		mViewport{ viewport },
		mUseBackFaceCulling{ useFaceCulling },
		mAlphaBlendMode{ alphaBlendMode },
		mColourBlendMode{ colourBlendMode },
		mDepthWrite{ depthWrite },
		mDepthTest{ depthTest },
		mPrimitiveType{ primitiveType }
	{}

	GraphicsPipelineDescription(const Shader& vert, const Shader& frag,
		const std::optional<Shader>& instancedVert,
		const std::optional<Shader>& geometryShader,
		const std::optional<Shader>& hullShaderr, const std::optional<Shader>& tesselationCOntrol,
		const Rect& scissor, const Rect& viewport,
		const bool useFaceCulling,
		const BlendMode alphaBlendMode,
		const BlendMode colourBlendMode,
		const bool depthWrite,
		const DepthTest depthTest,
		const Primitive primitiveType) :

		mVertexShader{ vert },
		mInstancedVertexShader{ instancedVert },
		mGeometryShader{ geometryShader },
		mHullShader{ hullShaderr },
		mTesselationControlShader{ tesselationCOntrol },
		mFragmentShader{ frag },
		mScissorRect{ scissor },
		mViewport{ viewport },
		mUseBackFaceCulling{ useFaceCulling },
		mAlphaBlendMode{ alphaBlendMode },
		mColourBlendMode{ colourBlendMode },
		mDepthWrite{ depthWrite },
		mDepthTest{ depthTest },
		mPrimitiveType{ primitiveType }
	{}
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
	GraphicsTask(const std::string& name, const GraphicsPipelineDescription& desc) : RenderTask{name}, mPipelineDescription{ desc }, mVertexAttributes{0}, mVertexBufferOffset{0} {}
	
	const GraphicsPipelineDescription& getPipelineDescription() const { return mPipelineDescription; }
	GraphicsPipelineDescription& getPipelineDescription() { return mPipelineDescription; }

	struct thunkedDraw {
		DrawType mDrawType;

		uint32_t mVertexOffset;
		uint32_t mNumberOfVerticies;
		uint32_t mIndexOffset;
		uint32_t mNumberOfIndicies;
		uint32_t mNumberOfInstances;
		std::string mIndirectBufferName;
		glm::mat4 mPushConstantValue;
	};

	void setVertexAttributes(int vertexAttributes)
		{ mVertexAttributes = vertexAttributes; }

	int getVertexAttributes() const { return mVertexAttributes; }

    void addDrawCall(const uint32_t vertexOffset, const uint32_t vertexCount)
	{ 
            mDrawCalls.push_back({DrawType::Standard, vertexOffset, vertexCount, 0, 0, 1, "", glm::mat4(1.0f)});
	}

    void addInstancedDraw(const uint32_t vertexOffset, const uint32_t vertexCount, const uint32_t instanceCount)
    {
        mDrawCalls.push_back({DrawType::Instanced, vertexOffset, vertexCount, 0, 0, instanceCount, "", glm::mat4(1.0f)});
    }

	void addIndexedDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies) 
	{ 
			mDrawCalls.push_back({ DrawType::Indexed, vertexOffset, 0, indexOffset, numberOfIndicies, 1, "", glm::mat4(1.0f) });
	}

    void addIndirectDrawCall(const uint32_t drawCalls, const std::string&& indirectBuffer)
    {
		mDrawCalls.push_back({DrawType::Indirect, 0, 0, 0, 0, drawCalls, indirectBuffer, glm::mat4(1.0f)});
    }

	void addIndexedInstancedDrawCall(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies)
	{
		mDrawCalls.push_back({ DrawType::IndexedInstanced, vertexOffset, 0, indexOffset, numberOfIndicies, numberOfInstances, "", glm::mat4(1.0f)});
	}

    void addIndexedIndirectDrawCall(const uint32_t drawCalls, const uint32_t indexOffset, const uint32_t numberOfIndicies, const std::string&& indirectName)
	{
		mDrawCalls.push_back({ DrawType::IndexedIndirect, 0, 0, indexOffset, numberOfIndicies, drawCalls, indirectName, glm::mat4(1.0f)});
	}

	void addPushConsatntValue(const glm::mat4& val)
	{
		mDrawCalls.push_back({DrawType::SetPushConstant, 0, 0, 0, 0, 0, "", val});
	}

    void recordCommands(Executor&, const RenderGraph&, const uint32_t taskIndex) const override final;

    std::vector<ClearValues> getClearValues() const;

	void mergeWith(const RenderTask&) override final;

	void clearCalls() override final { mDrawCalls.clear(); }

	void setVertexBufferOffset(const uint32_t offset)
	{ mVertexBufferOffset = offset; }

	uint32_t getVertexBufferOffset() const
	{ return mVertexBufferOffset; }

	uint32_t recordedCommandCount() const override final { return mDrawCalls.size(); }

	TaskType taskType() const override final { return TaskType::Graphics; }

    friend bool operator==(const GraphicsTask& lhs, const GraphicsTask& rhs);

private:

	std::vector<thunkedDraw> mDrawCalls;

	GraphicsPipelineDescription mPipelineDescription;
	
	int mVertexAttributes;

	uint32_t mVertexBufferOffset;
};


bool operator==(const GraphicsPipelineDescription& lhs, const GraphicsPipelineDescription& rhs);

#endif
