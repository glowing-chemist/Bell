#ifndef UBER_SHADER_STATE_CACHE_HPP
#define UBER_SHADER_STATE_CACHE_HPP

#include <cstdint>
#include <string>

class RenderEngine;
class Executor;
class Shader;
class RenderTask;
class RenderGraph;

class UberShaderStateCache
{
public:
    UberShaderStateCache(Executor* exec, const RenderGraph& graph, const RenderTask& task) :
        mExec{exec},
        mGraph{graph},
        mTask{task} {}
    virtual ~UberShaderStateCache() = default;

    virtual void update(const uint64_t) {}

protected:

    Executor* mExec;
    const RenderGraph& mGraph;
    const RenderTask& mTask;
};

class UberShaderMaterialStateCache : public UberShaderStateCache
{
public:
    UberShaderMaterialStateCache(Executor*, RenderEngine*, const RenderGraph& graph, const RenderTask& task, Shader& vertShader, const std::string& fragShader);
    ~UberShaderMaterialStateCache() = default;

    virtual void update(const uint64_t) override final;

private:

    RenderEngine* mEng;
    Shader& mVertexShader;
    std::string mFragmentShaderName;
    uint64_t mCurrentMaterialDefines;

};

#endif