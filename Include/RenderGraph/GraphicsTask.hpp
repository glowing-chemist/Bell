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
    Albedo = 1 << 6
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
    ClearValues(const AttachmentType type, const float red, const float green, const float blue, const float alpha) :
		r{ red }, g{ green }, b{ blue }, a{ alpha }, mType{type} {}

    float r, g, b, a;
	AttachmentType mType;
};


struct GraphicsPipelineDescription
{
	Rect mScissorRect;
	Rect mViewport;
	bool mUseBackFaceCulling;

	BlendMode mAlphaBlendMode;
	BlendMode mColourBlendMode;
	bool      mDepthWrite;
	DepthTest mDepthTest;

	Primitive mPrimitiveType;

    GraphicsPipelineDescription(const Rect& scissor, const Rect& viewport) :
		mScissorRect{ scissor },
		mViewport{ viewport },
		mUseBackFaceCulling{ true },
		mAlphaBlendMode{ BlendMode::None },
		mColourBlendMode{ BlendMode::None },
		mDepthWrite{ false },
		mDepthTest{ DepthTest::None },
		mPrimitiveType{ Primitive::TriangleList }
	{}

    GraphicsPipelineDescription(const Rect& scissor, const Rect& viewport,
		const bool useFaceCulling,
		const BlendMode alphaBlendMode,
		const BlendMode colourBlendMode,
		const bool depthWrite,
		const DepthTest depthTest,
        const Primitive primitiveType) :
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

// This class describes a renderpass with all of it's inputs and
// outputs.
class GraphicsTask : public RenderTask
{
public:
	GraphicsTask(const std::string& name, const GraphicsPipelineDescription& desc) : RenderTask{name}, mPipelineDescription{ desc }, mVertexAttributes{0}, mVertexBufferOffset{0} {}
	
	const GraphicsPipelineDescription& getPipelineDescription() const { return mPipelineDescription; }
	GraphicsPipelineDescription& getPipelineDescription() { return mPipelineDescription; }

	void setVertexAttributes(int vertexAttributes)
		{ mVertexAttributes = vertexAttributes; }

	int getVertexAttributes() const { return mVertexAttributes; }

    std::vector<ClearValues> getClearValues() const;

	void setVertexBufferOffset(const uint32_t offset)
	{ mVertexBufferOffset = offset; }

	uint32_t getVertexBufferOffset() const
	{ return mVertexBufferOffset; }

	TaskType taskType() const override final { return TaskType::Graphics; }

    friend bool operator==(const GraphicsTask& lhs, const GraphicsTask& rhs);

private:

	GraphicsPipelineDescription mPipelineDescription;
	
	int mVertexAttributes;

	uint32_t mVertexBufferOffset;
};


#endif
