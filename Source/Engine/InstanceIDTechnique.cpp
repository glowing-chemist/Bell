#include "Engine/InstanceIDTechnique.hpp"
#include "RenderGraph/GraphicsTask.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Core/Executor.hpp"

#include "Engine/Engine.hpp"

static constexpr char kSelectedInstanceBuffer[] = "SelectedInstanceBuffer";

InstanceIDTechnique::InstanceIDTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique("InstanceID", eng->getDevice()),
    mPipeline{Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                              getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         Rect{getDevice()->getSwapChain()->getSwapChainImageWidth(),
                              getDevice()->getSwapChain()->getSwapChainImageHeight()},
                         FaceWindingOrder::CW, BlendMode::None, BlendMode::None,
                         false, DepthTest::GreaterEqual, FillMode::Fill, Primitive::TriangleList},
                         mVertexShader(eng->getShader("./Shaders/InstanceID.vert")),
                         mFragmentShader(eng->getShader("./Shaders/InstanceID.frag")),
                         mComputeShader(eng->getShader("./Shaders/SetPickedInstance.comp")),
                         mSelectedIDBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::Uniform, sizeof(uint32_t), sizeof(uint32_t), "InstanceIDBuffer"),
                         mSelectedIDBufferView(mSelectedIDBuffer)
{
    GraphicsTask task{"InstanceID", mPipeline};
    task.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput("instanceInfo", AttachmentType::PushConstants);
    task.addManagedOutput(kInstanceIDBuffer, AttachmentType::RenderTarget2D, Format::R16UInt, SizeClass::Swapchain,
                          LoadOp::Clear_White);
    task.addOutput(kGBufferDepth, AttachmentType::Depth, Format::D32Float, LoadOp::Preserve);

    task.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                const RenderTask& task = graph.getTask(taskIndex);
                exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mVertexShader, nullptr, nullptr, nullptr, mFragmentShader);

                for(const auto* instance : meshes)
                {
                    const StaticMesh* mesh = instance->getMesh();
                    exec->bindVertexBuffer(*(mesh->getVertexBufferView()), 0);
                    exec->bindIndexBuffer(*(mesh->getIndexBufferView()), 0);

                    const std::vector<SubMesh>& subMeshes = mesh->getSubMeshes();
                    for(const auto& subMesh : subMeshes)
                    {
                        PushConstants constants{};
                        constants.transform = instance->getTransMatrix() * subMesh.mTransform;
                        constants.id = instance->getID();

                        exec->insertPushConstant(&constants, sizeof(PushConstants));
                        exec->indexedDraw(subMesh.mVertexOffset / mesh->getVertexStride(), subMesh.mIndexOffset / sizeof(uint32_t), subMesh.mIndexCount);
                    }
                }
            });

    mTaskID = graph.addTask(task);

    // Now pick selected instance from mouse position.
    ComputeTask pickerTask{"InstancePicker"};
    pickerTask.addInput(kInstanceIDBuffer, AttachmentType::Texture2D);
    pickerTask.addInput(kSelectedInstanceBuffer, AttachmentType::DataBufferWO);
    pickerTask.addInput("MousePos", AttachmentType::PushConstants);
    pickerTask.setRecordCommandsCallback(
            [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>& meshes)
            {
                const RenderTask& task = graph.getTask(taskIndex);
                exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mComputeShader);

                exec->insertPushConstant(&mCurrentMousePos, sizeof(uint2));
                exec->dispatch(1, 1, 1);
            });

    graph.addTask(pickerTask);

}


void InstanceIDTechnique::render(RenderGraph&, RenderEngine*)
{
    Buffer& currentBuffer = *mSelectedIDBuffer;
    MapInfo mapInfo{0, sizeof(uint32_t)};

    void* bufPtr = currentBuffer->map(mapInfo);
    memcpy(&mSelectedInstanceID, bufPtr, sizeof(uint32_t));
    currentBuffer->unmap();
}


void InstanceIDTechnique::bindResources(RenderGraph& graph)
{
    graph.bindBuffer(kSelectedInstanceBuffer, *mSelectedIDBufferView);
}