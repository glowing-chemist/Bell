#include "UberShaderStateCache.hpp"
#include "Engine/Engine.hpp"
#include "Core/Shader.hpp"
#include "Core/Executor.hpp"


UberShaderMaterialStateCache::UberShaderMaterialStateCache(Executor* exec,
                                                           RenderEngine* engine,
                                                           const RenderGraph& graph,
                                                           const RenderTask& task,
                                                           Shader &vertShader,
                                                           const std::string &fragShader) :
                                                           UberShaderStateCache(exec),
                                                           mEng(engine),
                                                           mGraph{graph},
                                                           mTask{task},
                                                           mVertexShader(vertShader),
                                                           mFragmentShaderName(fragShader),
                                                           mCurrentShadeFlags(~0ULL) {}


void UberShaderMaterialStateCache::update(const uint64_t shadeFlags)
{
    if(mCurrentShadeFlags != shadeFlags)
    {
        mCurrentShadeFlags = shadeFlags;

        ShaderDefine materialDefine(L"SHADE_FLAGS", mCurrentShadeFlags);
        Shader fragmentShader = mEng->getShader(mFragmentShaderName, materialDefine);
        mExec->setGraphicsShaders(static_cast<const GraphicsTask&>(mTask), mGraph, mVertexShader, nullptr, nullptr, nullptr, fragmentShader);
    }
}


UberShaderSkinnedStateCache::UberShaderSkinnedStateCache(Executor* exec, uint64_t* pipelines) :
        UberShaderStateCache(exec),
        mSkinned(false),
        mFirst(true)
{
    memcpy(mPipelines, pipelines, sizeof(PipelineHandle) * 2);
}


void UberShaderSkinnedStateCache::update(const uint64_t shadeFlags)
{
    const bool skinned = (shadeFlags & kShade_Skinning) > 0;
    if(skinned != mSkinned || mFirst)
    {
        mSkinned = skinned;
        mFirst = false;

        mExec->setGraphicsPipeline(mPipelines[size_t(mSkinned)]);
    }
}


UberShaderCachedPipelineStateCache::UberShaderCachedPipelineStateCache(Executor* exec, std::unordered_map<uint64_t, uint64_t>& pipelineCache) :
    UberShaderStateCache(exec),
    mPipelineHandles(pipelineCache),
    mCurrentShadeFlags(~0ULL)
{}


void UberShaderCachedPipelineStateCache::update(const uint64_t shadeFlags)
{
    if(mCurrentShadeFlags != shadeFlags)
    {
        mCurrentShadeFlags = shadeFlags;

        BELL_ASSERT(mPipelineHandles.find(mCurrentShadeFlags) != mPipelineHandles.end(), "Pipeline not cached")
        mExec->setGraphicsPipeline(mPipelineHandles[mCurrentShadeFlags]);
    }
}