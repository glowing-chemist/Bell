#include "Engine/OcclusionCullingTechnique.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Engine/Engine.hpp"

#include "Core/Executor.hpp"
#include "Core/BarrierManager.hpp"

constexpr const char kOcclusionIndexBuffer[] = "OcclusionIndexBuffer";
constexpr const char kOcclusionSampler[] = "OcclusionSampler";


OcclusionCullingTechnique::OcclusionCullingTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique("Occlusion culling", eng->getDevice()),
    mOcclusionCullingShader(eng->getShader("./Shaders/OcclusionCulling.comp")),
    mBoundsIndexBuffer(getDevice(), BufferUsage::TransferDest | BufferUsage::DataBuffer, sizeof(uint32_t) * 500, sizeof(uint32_t) * 500, "Occlusion index buffer"),
    mBoundsIndexBufferView(mBoundsIndexBuffer),
    mPredicationBuffer(getDevice(), BufferUsage::CommandPredication| BufferUsage::DataBuffer, sizeof(uint32_t) * 500, sizeof(uint32_t) * 500, "Occlusion index buffer"),
    mPredicationBufferView(mPredicationBuffer),
    mOcclusionSampler(SamplerType::Point)
{
    BELL_ASSERT(getDevice()->getHasCommandPredicationSupport(), "Device does not have conditional rendering support")

    mOcclusionSampler.setAddressModeU(AddressMode::Clamp);
    mOcclusionSampler.setAddressModeV(AddressMode::Clamp);

    ComputeTask task{"Occlusion culling"};
    task.addInput(kOcclusionIndexBuffer, AttachmentType::DataBufferRO);
    task.addInput(kMeshBoundsBuffer, AttachmentType::DataBufferRO);
    task.addInput(kOcclusionPredicationBuffer, AttachmentType::DataBufferWO);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kPreviousLinearDepth, AttachmentType::Texture2D);
    task.addInput(kOcclusionSampler, AttachmentType::Sampler);
    task.addInput("MeshCount", AttachmentType::PushConstants);
    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
        {
            PROFILER_EVENT("Occlusion culling");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("Occlusion culling");

            if(!meshes.empty() && !eng->getDebugCameraActive())
            {
                std::vector<uint32_t> indicies{};
                indicies.reserve(meshes.size());
                for(const auto& mesh : meshes)
                {
                    indicies.push_back(eng->getMeshBoundsIndex(mesh));
                }

                exec->copyDataToBuffer(indicies.data(), sizeof(uint32_t) * indicies.size(), 0, *mBoundsIndexBuffer);

                const RenderTask& task = graph.getTask(taskIndex);
                exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mOcclusionCullingShader);

                // Make sure uploaded data is visible.
                BarrierRecorder recorder{eng->getDevice()};
                recorder->memoryBarrier(*mBoundsIndexBuffer, Hazard::ReadAfterWrite, SyncPoint::TransferSource, SyncPoint::ComputeShader);

                exec->recordBarriers(recorder);

                {
                    const uint32_t meshCount = meshes.size();

                    exec->insertPushConstant(&meshCount, sizeof(uint32_t));
                    exec->dispatch(std::ceil(meshCount / 64.0f), 1, 1);
                }
            }
        }
   );

    graph.addTask(task);
}


void OcclusionCullingTechnique::bindResources(RenderGraph& graph)
{
    graph.bindBuffer(kOcclusionIndexBuffer, *mBoundsIndexBufferView);

    if(!graph.isResourceSlotBound(kOcclusionPredicationBuffer))
    {
        graph.bindBuffer(kOcclusionPredicationBuffer, mPredicationBufferView);
        graph.bindSampler(kOcclusionSampler, mOcclusionSampler);
    }
}
