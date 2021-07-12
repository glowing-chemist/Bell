#ifndef UBER_SHADER_STATE_CACHE_HPP
#define UBER_SHADER_STATE_CACHE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

class RenderEngine;
class Executor;
class Shader;
class RenderTask;
class RenderGraph;

class UberShaderStateCache
{
public:
    UberShaderStateCache(Executor* exec) :
        mExec{exec} {}
    virtual ~UberShaderStateCache() = default;

    virtual void update(const uint64_t) {}

protected:

    Executor* mExec;
};

class UberShaderMaterialStateCache : public UberShaderStateCache
{
public:
    UberShaderMaterialStateCache(Executor*, RenderEngine*, const RenderGraph& graph, const RenderTask& task, Shader& vertShader, const std::string& fragShader);
    ~UberShaderMaterialStateCache() = default;

    virtual void update(const uint64_t) override final;

private:

    RenderEngine* mEng;
    const RenderGraph& mGraph;
    const RenderTask& mTask;
    Shader& mVertexShader;
    std::string mFragmentShaderName;
    uint64_t mCurrentShadeFlags;

};

class UberShaderCachedPipelineStateCache : public UberShaderStateCache
{
public:
    UberShaderCachedPipelineStateCache(Executor*, std::unordered_map<uint64_t, uint64_t>& cache);
    ~UberShaderCachedPipelineStateCache() = default;

    virtual void update(const uint64_t) override final;

private:

    std::unordered_map<uint64_t, uint64_t>& mPipelineHandles;
    uint64_t mCurrentShadeFlags;

};

#endif