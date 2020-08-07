#include "Engine/VisualizeLightProbesTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"

#include "glm/gtx/transform.hpp"


const char kLightProbeVertexBuffer[] = "LightProbesVert";
const char kLightProbeIndexBuffer[] = "LightProbeIndex";


ViaualizeLightProbesTechnique::ViaualizeLightProbesTechnique(Engine* eng, RenderGraph& graph) :
    Technique("VisualizeLightProbes", eng->getDevice()),
    mIrradianceProbePipeline{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                getDevice()->getSwapChain()->getSwapChainImageHeight()},
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight()},
          true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList},
    mIrradianceVolumePipeline{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                getDevice()->getSwapChain()->getSwapChainImageHeight()},
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight()},
          true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList},
    mLightProbeVisVertexShader(eng->getShader("./Shaders/LightProbeVis.vert")),
    mLigthProbeVisFragmentShader(eng->getShader("./Shaders/LightProbeVis.frag")),
    mLigthVolumeVisFragmentShader(eng->getShader("./Shaders/LightProbeVolumeVis.frag")),
    mTaskID{-1},
    mVertexBuffer(eng->getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest, eng->getUnitSphereMesh().getVertexData().size(),
                  eng->getUnitSphereMesh().getVertexData().size(), "LightProbe vertex buffer"),
    mIndexBuffer(eng->getDevice(), BufferUsage::Index | BufferUsage::TransferDest, eng->getUnitSphereMesh().getIndexData().size() * sizeof(uint32_t),
                  eng->getUnitSphereMesh().getIndexData().size() * sizeof(uint32_t), "LightProbe index buffer")
{

    if(!eng->getIrradianceProbes().empty())
    {
        GraphicsTask task{"Visualize irradiance probes", mIrradianceProbePipeline};
        task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals);
        task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
        task.addInput(kDefaultSampler, AttachmentType::Sampler);
        task.addInput(kLightProbes, AttachmentType::ShaderResourceSet);
        task.addInput("LightProbePushConstants", AttachmentType::PushConstants);
        task.addInput(kLightProbeVertexBuffer, AttachmentType::VertexBuffer);
        task.addInput(kLightProbeIndexBuffer, AttachmentType::IndexBuffer);
        task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
        task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);
        task.setRecordCommandsCallback(
                    [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        struct PushConstants
                        {
                            float4x4 transform;
                            uint32_t index;
                        };

                        exec->bindIndexBuffer(mIndexBuffer, 0);
                        exec->bindVertexBuffer(mVertexBuffer, 0);

                        const RenderTask& task = graph.getTask(taskIndex);
                        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mLightProbeVisVertexShader, nullptr, nullptr, nullptr, mLigthProbeVisFragmentShader);

                        const float3 sceneSize = eng->getScene()->getBounds().getSideLengths();
                        float maxScale = 0.01f * std::max(sceneSize.x, std::max(sceneSize.y, sceneSize.z));

                        const auto& probes = eng->getIrradianceProbes();
                        uint32_t index = 0;
                        for(const auto& probe : probes)
                        {
                            PushConstants pc{};
                            pc.transform = glm::translate(float4x4(1.0f), float3(probe.mPosition)) * glm::scale(float3(maxScale, maxScale, maxScale));
                            pc.index = index++;

                            exec->insertPushConsatnt(&pc, sizeof(PushConstants));
                            exec->indexedDraw(0, 0, mIndexCount);
                        }
                    }
        );

        mTaskID = graph.addTask(task);
    }
    else // we're only rendering volumes in the editor.
    {
        GraphicsTask task{"Visualize irradiance volumes", mIrradianceVolumePipeline};
        task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals);
        task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
        task.addInput("LightProbePushConstants", AttachmentType::PushConstants);
        task.addInput(kLightProbeVertexBuffer, AttachmentType::VertexBuffer);
        task.addInput(kLightProbeIndexBuffer, AttachmentType::IndexBuffer);
        task.addOutput(kGlobalLighting, AttachmentType::RenderTarget2D, Format::RGBA8UNorm);
        task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float);
        task.setRecordCommandsCallback(
                    [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                    {
                        struct PushConstants
                        {
                            float4x4 transform;
                            uint32_t index;
                        };

                        exec->bindIndexBuffer(mIndexBuffer, 0);
                        exec->bindVertexBuffer(mVertexBuffer, 0);

                        const RenderTask& task = graph.getTask(taskIndex);
                        exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mLightProbeVisVertexShader, nullptr, nullptr, nullptr, mLigthVolumeVisFragmentShader);

                        const float3 sceneSize = eng->getScene()->getBounds().getSideLengths();
                        float maxScale = 0.01f * std::max(sceneSize.x, std::max(sceneSize.y, sceneSize.z));

                        const auto& volumes = eng->getIrradianceVolumes();
                        for(const auto& volume : volumes)
                        {
                            std::vector<float3> probePositions = volume.getProbePositions();
                            for(const auto& position : probePositions)
                            {
                                PushConstants pc{};
                                pc.transform = glm::translate(float4x4(1.0f), float3(position)) * glm::scale(float3(maxScale, maxScale, maxScale));
                                pc.index = 0;

                                exec->insertPushConsatnt(&pc, sizeof(PushConstants));
                                exec->indexedDraw(0, 0, mIndexCount);
                            }
                        }
                    }
        );

        mTaskID = graph.addTask(task);

        // Add a task to draw box around volumes.

    }

    // upload vertex and iundex data.
    const StaticMesh& unitSphere = eng->getUnitSphereMesh();
    const auto& vertexData = unitSphere.getVertexData();
    mVertexBuffer->setContents(vertexData.data(), vertexData.size());

    const auto& indexData = unitSphere.getIndexData();
    mIndexBuffer->setContents(indexData.data(), indexData.size() * sizeof(uint32_t));
    mIndexCount = indexData.size();
}

