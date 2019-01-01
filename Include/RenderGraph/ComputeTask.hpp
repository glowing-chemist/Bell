#ifndef COMPUTETASK_HPP
#define COMPUTETASK_HPP

#include "RenderTask.hpp"

#include <string>
#include <vector>
#include <cstdint>

enum class DispatchType
{
	Standard,
	Indirect
};

struct ComputePipelineDescription 
{
	std::string mComputeShader;
};
// needed in order to use unordered_map
namespace std
{
	template<>
	struct hash<ComputePipelineDescription>
	{
		size_t operator()(const ComputePipelineDescription&) const noexcept;
	};
}


// This class describes a compute task (sync and async)
class ComputeTask : public RenderTask 
{
public:
	ComputeTask(const std::string& name, ComputePipelineDescription desc) : mName{ name }, mPipelineDescription{ desc } {}

	void addDispatch(const uint32_t x, const uint32_t y, const uint32_t z) { mComputeCalls.push_back({DispatchType::Standard, x, y, z}); }
	void addIndirectDispatch(const uint32_t x, const uint32_t y, const uint32_t z) { mComputeCalls.push_back({DispatchType::Indirect, x, y, z}); }

	void clearCalls() override { mComputeCalls.clear(); }

private:
	std::string mName;

	struct thunkedCompute
	{
		DispatchType mDispatchType;
		uint32_t x, y, z;
	};
	std::vector<thunkedCompute> mComputeCalls;

	ComputePipelineDescription mPipelineDescription;

};

#endif
