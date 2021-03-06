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
                                                           UberShaderStateCache(exec, graph, task),
                                                           mEng(engine),
                                                           mVertexShader(vertShader),
                                                           mFragmentShaderName(fragShader),
                                                           mCurrentMaterialDefines(~0ULL) {}


void UberShaderMaterialStateCache::update(const uint64_t materialDefines)
{
    if(mCurrentMaterialDefines != materialDefines)
    {
        mCurrentMaterialDefines = materialDefines;

        ShaderDefine materialDefine("MATERIAL_FLAGS", mCurrentMaterialDefines);
        Shader fragmentShader = mEng->getShader(mFragmentShaderName, materialDefine);
        mExec->setGraphicsShaders(static_cast<const GraphicsTask&>(mTask), mGraph, mVertexShader, nullptr, nullptr, nullptr, fragmentShader);
    }
}