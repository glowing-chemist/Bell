#include "Engine/VisualizeLightProbesTechnique.hpp"

#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "Core/Executor.hpp"

#include "glm/gtx/transform.hpp"


const char kLightProbeVertexBuffer[] = "LightProbesVert";
const char kLightProbeIndexBuffer[] = "LightProbeIndex";


ViaualizeLightProbesTechnique::ViaualizeLightProbesTechnique(Engine* eng, RenderGraph& graph) :
    Technique("VisualizeLightProbes", eng->getDevice()),
    mDesc{eng->getShader("./Shaders/LightProbeVis.vert"),
          eng->getShader("./Shaders/LightProbeVis.frag"),
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                getDevice()->getSwapChain()->getSwapChainImageHeight()},
          Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
          getDevice()->getSwapChain()->getSwapChainImageHeight()},
          true, BlendMode::None, BlendMode::None, false, DepthTest::GreaterEqual, Primitive::TriangleList},
    mTaskID{-1},
    mVertexBuffer(eng->getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest, eng->getUnitSphereMesh().getVertexData().size(),
                  eng->getUnitSphereMesh().getVertexData().size(), "LightProbe vertex buffer"),
    mIndexBuffer(eng->getDevice(), BufferUsage::Index | BufferUsage::TransferDest, eng->getUnitSphereMesh().getIndexData().size() * sizeof(uint32_t),
                  eng->getUnitSphereMesh().getIndexData().size() * sizeof(uint32_t), "LightProbe index buffer")
{
    GraphicsTask task{"Visualize irradiance probes", mDesc};
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
                [this](Executor* exec, Engine* eng, const std::vector<const MeshInstance*>&)
                        {
                            struct PushConstants
                            {
                                float4x4 transform;
                                uint32_t index;
                            };

                            exec->bindIndexBuffer(mIndexBuffer, 0);
                            exec->bindVertexBuffer(mVertexBuffer, 0);

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

    // upload vertex and iundex data.
    const StaticMesh& unitSphere = eng->getUnitSphereMesh();
    const auto& vertexData = unitSphere.getVertexData();
    mVertexBuffer->setContents(vertexData.data(), vertexData.size());

    const auto& indexData = unitSphere.getIndexData();
    mIndexBuffer->setContents(indexData.data(), indexData.size() * sizeof(uint32_t));
    mIndexCount = indexData.size();
}

