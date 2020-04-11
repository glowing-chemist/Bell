#include "RenderGraph/ComputeTask.hpp"
#include "RenderGraph/RenderGraph.hpp"

namespace std
{

    size_t hash<ComputePipelineDescription>::operator()(const ComputePipelineDescription& desc) const noexcept
    {
        std::hash<std::string> stringHasher{};

        size_t hash = 0;
		hash += stringHasher(desc.mComputeShader->getFilePath());


        return hash;
    }

}


bool operator==(const ComputeTask& lhs, const ComputeTask& rhs)
{
    std::hash<ComputePipelineDescription> hasher{};
    return hasher(lhs.getPipelineDescription()) == hasher(rhs.getPipelineDescription());
}


bool operator==(const ComputePipelineDescription& lhs, const ComputePipelineDescription& rhs)
{
    std::hash<ComputePipelineDescription> hasher{};
    return hasher(lhs) == hasher(rhs);
}
