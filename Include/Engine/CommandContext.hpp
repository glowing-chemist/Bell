#ifndef COMMANDCONTEXT_HPP
#define COMMANDCONTEXT_HPP

#include "Core/ImageView.hpp"
#include "Core/BufferView.hpp"
#include "Core/Sampler.hpp"

#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"


#include <array>
#include <ctype.h>
#include <set>


enum class ContextType {
    Graphics,
    Compute
};


class CommandContext
{

public:
    CommandContext();
    ~CommandContext() = default;


    void setActiveContextType(const ContextType type)
    { mCurrentContextType = type; }

    CommandContext& setGraphicsPipelineState(const GraphicsPipelineDescription&);
    CommandContext& setCompuetPipelineState(const ComputePipelineDescription&);

    // Chainable functions for binding resources
	CommandContext& bindImageViews(const ImageView* view, const char* const* slots, const uint32_t start, const uint32_t count);
	CommandContext& bindImageViewArrays(const ImageViewArray* view, const char* const* slots, const uint32_t start, const uint32_t count);
	CommandContext& bindStorageTextureViews(const ImageView* view, const char* const * slots, const uint32_t start, const uint32_t count);
	CommandContext& bindUniformBufferViews(const BufferView* view, const char* const * slots, const uint32_t start, const uint32_t count);
	CommandContext& bindStorageBufferViews(const BufferView* view, const char* const * slots, const uint32_t start,const uint32_t count);
	CommandContext& bindSamplers(const Sampler* view, const char* const * slots, const uint32_t start, const uint32_t count);
	CommandContext& bindRenderTargets(const ImageView* view, const char* const * slots, const LoadOp*, const uint32_t start, const uint32_t count);
	CommandContext& bindDepthStencilView(const ImageView* view, const char* const* slots, const LoadOp*, const uint32_t start, const uint32_t count);
	CommandContext& setVertexAttributes(const int attr)
    { mCurrentVertexAttributes = attr; return *this; }

    // Functions that record graphics commands
    void draw(const uint32_t vertexOffset, const uint32_t numberOfVerticies);
    void drawIndexed(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfIndicies);
    void drawIndirect(const uint32_t drawCalls, const std::string&& indirectBuffer);
    void drawIndexedInstanced(const uint32_t vertexOffset, const uint32_t indexOffset, const uint32_t numberOfInstances, const uint32_t numberOfIndicies);
    void drawIndexedIndirect(const uint32_t drawCalls, const uint32_t indexOffset, const uint32_t numberOfIndicies, const std::string&& indirectName);

    // Functions for
    CommandContext& setPushConstantsEnable(const bool);
    void setPushConstants(const glm::mat4&);

    // Functions that record compute commands
    void dispatch(const uint32_t x, const uint32_t y, const uint32_t z);
    void dispatchIndirect(/* TODO */) { BELL_TRAP; }


    void resetBindings();
    void reset();


    RenderGraph& finialise();

private:

    void addTaskToGraph();

    // Keeps track of whether the context is building new resource state or recording commands.
    enum class RecordingState
    {
	Resources,
	Commands
    };

    ContextType mCurrentContextType;
    RecordingState mCurrentRecordingState;

    std::optional<GraphicsPipelineDescription> mCurrentGraphicsPipelineState;
    std::optional<ComputePipelineDescription> mCurrentComputePipelineState;

    // current task resource state
    struct BindingInfo
    {
        std::string mName; // also slot
        AttachmentType mType;
        bool mBound;
    };
    std::array<BindingInfo, 16> mCurrentResourceBindings;

    struct RenderTargetInfo
    {
        std::string mName; // also slot
        AttachmentType mType;
        Format mFormat;
        LoadOp mLoadOp;
        bool mBound;
    };
    std::array<RenderTargetInfo, 16> mCurrentFrameBuffer;

    bool mPushConstantsEnabled;

    int mCurrentVertexAttributes;

    // Use the same system that the graphics/compute tasks use for "thunking" draw calls.
    // This is done here in order to avoid having to allocate a task.
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
    std::vector<thunkedDraw> mThunkedDrawCalls;

    struct thunkedCompute
    {
        DispatchType mDispatchType;
        uint32_t x, y, z;
        std::string mIndirectBuffer;
    };
    std::vector<thunkedCompute> mThunkedDispatches;

    // All resources needed for the rendergraph.
    template<typename T>
    struct ResourceComparator
    {
        bool operator()(const std::pair<std::string, T>& lhs, const std::pair<std::string, T>& rhs) const noexcept
        {
            return lhs.first < rhs.first;
        }
    };

    std::set<std::pair<std::string, ImageView>, ResourceComparator<ImageView>> mImages;
    std::set<std::pair<std::string, ImageViewArray>, ResourceComparator<ImageViewArray>> mImageArrays;
    std::set<std::pair<std::string, BufferView>, ResourceComparator<BufferView>> mBuffers;
    std::set<std::pair<std::string, Sampler>, ResourceComparator<Sampler>>    mSamplers;

    RenderGraph mRenderGraph;
};


#endif
