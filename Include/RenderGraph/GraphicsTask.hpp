#ifndef GRAPHICSTASK_HPP
#define GRAPHICSTASK_HPP

#include "RenderTask.hpp"
#include "Core/Shader.hpp"

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
    Albedo = 1 << 6,
	Material = 1 << 7
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

	GraphicsPipelineDescription(const Shader& vert, const Shader& frag,
								const Rect& scissor, const Rect& viewport) :
		mVertexShader{vert},
		mGeometryShader{},
		mHullShader{},
		mTesselationControlShader{},
		mFragmentShader{frag},
		mScissorRect{scissor},
		mViewport{viewport},
		mUseBackFaceCulling{false},
		mAlphaBlendMode{BlendMode::None},
		mColourBlendMode{BlendMode::None},
		mDepthTest{DepthTest::None}
	{}

	GraphicsPipelineDescription(const Shader& vert, const Shader& frag,
								const Rect& scissor, const Rect& viewport,
								const bool useFaceCulling,
								const BlendMode alphaBlendMode,
								const BlendMode colourBlendMode,
								const DepthTest depthTest) :
		mVertexShader{vert},
		mGeometryShader{},
		mHullShader{},
		mTesselationControlShader{},
		mFragmentShader{frag},
		mScissorRect{scissor},
		mViewport{viewport},
		mUseBackFaceCulling{useFaceCulling},
		mAlphaBlendMode{alphaBlendMode},
		mColourBlendMode{colourBlendMode},
		mDepthTest{depthTest}
	{}

	GraphicsPipelineDescription(const Shader& vert, const Shader& frag,
								const std::optional<Shader>& geometryShader,
								const std::optional<Shader>& hullShaderr, const std::optional<Shader>& tesselationCOntrol,
								const Rect& scissor, const Rect& viewport,
								const bool useFaceCulling,
								const BlendMode alphaBlendMode,
								const BlendMode colourBlendMode,
								const DepthTest depthTest) :
		mVertexShader{vert},
		mGeometryShader{geometryShader},
		mHullShader{hullShaderr},
		mTesselationControlShader{tesselationCOntrol},
		mFragmentShader{frag},
		mScissorRect{scissor},
		mViewport{viewport},
		mUseBackFaceCulling{useFaceCulling},
		mAlphaBlendMode{alphaBlendMode},
		mColourBlendMode{colourBlendMode},
		mDepthTest{depthTest}
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
    GraphicsTask(const std::string& name, const GraphicsPipelineDescription& desc) : RenderTask{name}, mPipelineDescription{ desc } {}
	
	GraphicsPipelineDescription getPipelineDescription() const { return mPipelineDescription; }

	void setVertexAttributes(int vertexAttributes)
		{ mVertexAttributes = vertexAttributes; }

	int getVertexAttributes() const { return mVertexAttributes; }

	void addDrawCall(const uint32_t vertexOffset, const uint32_t numberOfVerticies) 
	{ 
			mDrawCalls.push_back({DrawType::Standard, vertexOffset, numberOfVerticies, 0, 0, 1, "", glm::mat4(1.0f)});
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

	void recordCommands(vk::CommandBuffer, const RenderGraph&, const vulkanResources&) const override final;

    std::vector<vk::ClearValue> getClearValues() const;

	void mergeWith(const RenderTask&) override final;

	void clearCalls() override final { mDrawCalls.clear(); }

	uint32_t recordedCommandCount() const override final { return mDrawCalls.size(); }

	TaskType taskType() const override final { return TaskType::Graphics; }

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
		glm::mat4 mPushConstantValue;
	};

	std::vector<thunkedDraw> mDrawCalls;

	GraphicsPipelineDescription mPipelineDescription;
	
	int mVertexAttributes;
};


bool operator==(const GraphicsPipelineDescription& lhs, const GraphicsPipelineDescription& rhs);

#endif
