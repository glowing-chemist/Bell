#include "Engine/ShadowMappingTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Engine/DefaultResourceSlots.hpp"
#include "Engine/UtilityTasks.hpp"
#include "Core/Executor.hpp"

const char kShadowMapRaw[] = "ShadowMapRaw";
extern const char kShadowMapCounter[] = "ShadowMap Counter";
const char kShadowMapDepth[] = "ShadowMapDepth";
const char kShadowMapBlurIntermediate[] = "ShadowMapBlurIntermediate";
const char kShadowMapBlured[] = "ShadowMapBlured";
extern const char kShadowMapUpsamped[] = "ShaowMapupsampled";
extern const char kShadowMapHistory[] = "ShadowMapHistory";



ShadowMappingTechnique::ShadowMappingTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique("ShadowMapping", eng->getDevice()),
    mDesc(Rect{static_cast<uint32_t>(eng->getShadowMapResolution().x),
          static_cast<uint32_t>(eng->getShadowMapResolution().y)},
          Rect{static_cast<uint32_t>(eng->getShadowMapResolution().x),
          static_cast<uint32_t>(eng->getShadowMapResolution().y)},
        FaceWindingOrder::CW, BlendMode::None, BlendMode::None, true, DepthTest::LessEqual, FillMode::Fill, Primitive::TriangleList),
    mShadowMapVertexShader(eng->getShader("./Shaders/ShadowMap.vert")),
    mShadowMapFragmentShader(eng->getShader("./Shaders/VarianceShadowMap.frag")),
    mBlurXShader( eng->getShader("Shaders/blurXrg32f.comp") ),
    mBlurYShader( eng->getShader("Shaders/blurYrg32f.comp") ),
    mResolveShader(eng->getShader("./Shaders/ResolveVarianceShadowMap.comp")),

    mShadowMapDepth(getDevice(), Format::D32Float, ImageUsage::DepthStencil, eng->getShadowMapResolution().x,
                                                                                        eng->getShadowMapResolution().y,
               1, 1, 1, 1, "ShadowMapDepth"),
    mShadowMapDepthView(mShadowMapDepth, ImageViewType::Depth),

    mShadowMapRaw(getDevice(), Format::RG32Float, ImageUsage::ColourAttachment | ImageUsage::Sampled, eng->getShadowMapResolution().x,
                                                                                        eng->getShadowMapResolution().y,
               1, 1, 1, 1, "ShadowMapRaw"),
    mShadowMapRawView(mShadowMapRaw, ImageViewType::Colour),

    mShadowMap(getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, eng->getSwapChainImage()->getExtent(0, 0).width,
                                                                                        eng->getSwapChainImage()->getExtent(0, 0).height,
               1, 1, 1, 1, "ShadowMap"),
    mShadowMapView(mShadowMap, ImageViewType::Colour),
    mShadowMapIntermediate(getDevice(), Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, eng->getShadowMapResolution().x,
                                                                                                      eng->getShadowMapResolution().y,
               1, 1, 1, 1, "ShadowMapIntermediate"),
    mShadowMapIntermediateView(mShadowMapIntermediate, ImageViewType::Colour),
    mShadowMapBlured(getDevice(), Format::RG32Float, ImageUsage::Storage | ImageUsage::Sampled, eng->getShadowMapResolution().x,
                                                                                                    eng->getShadowMapResolution().y,
               1, 1, 1, 1, "ShadowMapBlured"),
    mShadowMapBluredView(mShadowMapBlured, ImageViewType::Colour)
{
    GraphicsTask shadowTask{ "ShadowMapping", mDesc };
    shadowTask.setVertexAttributes(VertexAttributes::Position4 |
                              VertexAttributes::Normals |VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

    shadowTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    shadowTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    shadowTask.addInput(kMaterials, AttachmentType::ShaderResourceSet);
    shadowTask.addInput("lightMatrix", AttachmentType::PushConstants);
    shadowTask.addInput(kSceneVertexBuffer, AttachmentType::VertexBuffer);
    shadowTask.addInput(kSceneIndexBuffer, AttachmentType::IndexBuffer);

    shadowTask.addOutput(kShadowMapRaw, AttachmentType::RenderTarget2D, Format::RG32Float, LoadOp::Clear_Float_Max);
    shadowTask.addOutput(kShadowMapDepth, AttachmentType::Depth, Format::D32Float, LoadOp::Clear_White, StoreOp::Discard);
    mShadowTask = graph.addTask(shadowTask);

    ComputeTask blurXTask{ "ShadowMapBlurX" };
    blurXTask.addInput(kShadowMapRaw, AttachmentType::Texture2D);
    blurXTask.addInput(kShadowMapBlurIntermediate, AttachmentType::Image2D);
    blurXTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mBlurXTaskID = graph.addTask(blurXTask);

    ComputeTask blurYTask{ "ShadowMapBlurY" };
    blurYTask.addInput(kShadowMapBlurIntermediate, AttachmentType::Texture2D);
    blurYTask.addInput(kShadowMapBlured, AttachmentType::Image2D);
    blurYTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mBlurYTaskID = graph.addTask(blurYTask);

    ComputeTask resolveTask{ "Resolve shadow mapping" };
    resolveTask.addInput(kGBufferDepth, AttachmentType::Texture2D);
    resolveTask.addInput(kShadowMapBlured, AttachmentType::Texture2D);
    resolveTask.addInput(kShadowMap, AttachmentType::Image2D);
    resolveTask.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    resolveTask.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    resolveTask.addInput(kDefaultSampler, AttachmentType::Sampler);
    mResolveTaskID = graph.addTask(resolveTask);
}


void ShadowMappingTechnique::render(RenderGraph& graph, RenderEngine*)
{
    (mShadowMapDepth)->updateLastAccessed();
    (mShadowMapDepthView)->updateLastAccessed();
    (mShadowMapRaw)->updateLastAccessed();
    (mShadowMapRawView)->updateLastAccessed();
    (mShadowMap)->updateLastAccessed();
    (mShadowMapView)->updateLastAccessed();
    (mShadowMapIntermediate)->updateLastAccessed();
    (mShadowMapIntermediateView)->updateLastAccessed();
    (mShadowMapBlured)->updateLastAccessed();
    (mShadowMapBluredView)->updateLastAccessed();

    GraphicsTask& shadowTask = static_cast<GraphicsTask&>(graph.getTask(mShadowTask));
    ComputeTask& blurXTask = static_cast<ComputeTask&>(graph.getTask(mBlurXTaskID));
    ComputeTask& blurYTask = static_cast<ComputeTask&>(graph.getTask(mBlurYTaskID));
    ComputeTask& resolveTask = static_cast<ComputeTask&>(graph.getTask(mResolveTaskID));

    shadowTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            PROFILER_EVENT("Render shadow maps");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("Render shadow maps");

            const Frustum lightFrustum = eng->getScene()->getShadowingLightFrustum();
            std::vector<const MeshInstance*> meshes = eng->getScene()->getVisibleMeshes(lightFrustum);
            std::sort(meshes.begin(), meshes.end(), [lightPosition = eng->getScene()->getShadowingLight().mPosition](const MeshInstance* lhs, const MeshInstance* rhs)
            {
                const float3 centralLeft = lhs->getMesh()->getAABB().getCentralPoint();
                const float leftDistance = glm::length(centralLeft - float3(lightPosition));

                const float3 centralright = rhs->getMesh()->getAABB().getCentralPoint();
                const float rightDistance = glm::length(centralright - float3(lightPosition));

                return leftDistance < rightDistance;
            });

            exec->bindIndexBuffer(eng->getIndexBuffer(), 0);
            exec->bindVertexBuffer(eng->getVertexBuffer(), 0);

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setGraphicsShaders(static_cast<const GraphicsTask&>(task), graph, mShadowMapVertexShader, nullptr, nullptr, nullptr, mShadowMapFragmentShader);

            UberShaderStateCache stateCache(exec);

            for (const auto& mesh : meshes)
            {
                // Don't render transparent geometry.
                if (!(mesh->getInstanceFlags() & InstanceFlags::Draw))
                    continue;

                const auto [vertexOffset, indexOffset] = eng->addMeshToBuffer(mesh->getMesh());

                mesh->draw(exec, &stateCache, vertexOffset, indexOffset);
            }
        }
    );

    blurXTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mBlurXShader);

            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

            exec->dispatch(std::ceil(threadGroupWidth / 128.0f), threadGroupHeight, 1.0f);
        }
    );

    blurYTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mBlurYShader);

            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

            exec->dispatch(threadGroupWidth, std::ceil(threadGroupHeight / 128.0f), 1.0f);
        }
    );

    resolveTask.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            PROFILER_EVENT("Ressolve shadow maps");
            PROFILER_GPU_TASK(exec);
            PROFILER_GPU_EVENT("Ressolve shadow maps");

            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mResolveShader);

            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height;

            exec->dispatch( static_cast<uint32_t>(std::ceil(threadGroupWidth / 32.0f)),
                            static_cast<uint32_t>(std::ceil(threadGroupHeight / 32.0f)),
                            1);
        }
    );
}


RayTracedShadowsTechnique::RayTracedShadowsTechnique(RenderEngine* eng, RenderGraph& graph) :
                Technique("RayTracedShadows", eng->getDevice()),
                mFirstFrame(true),
                mShadowMapCounter(getDevice(), Format::R32Uint, ImageUsage::Storage | ImageUsage::TransferDest, getDevice()->getSwapChain()->getSwapChainImageWidth() / 4, getDevice()->getSwapChain()->getSwapChainImageHeight() / 4,
                           1, 1, 1, 1, "ShadowMapCounter"),
                mShadowMapCounterView(mShadowMapCounter, ImageViewType::Colour),
                mShadowMapHistory{Image{getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth() / 4, getDevice()->getSwapChain()->getSwapChainImageHeight() / 4,
                           1, 1, 1, 1, "ShadowMapHistory1"},
                                  Image{getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled | ImageUsage::TransferDest, getDevice()->getSwapChain()->getSwapChainImageWidth() / 4, getDevice()->getSwapChain()->getSwapChainImageHeight() / 4,
                                                             1, 1, 1, 1, "ShadowMapHistory2"}},
                mShadowMapHistoryView{ImageView{mShadowMapHistory[0], ImageViewType::Colour}, ImageView{mShadowMapHistory[1], ImageViewType::Colour}},
                mShadowMap(getDevice(), Format::R8UNorm, ImageUsage::Storage | ImageUsage::Sampled, getDevice()->getSwapChain()->getSwapChainImageWidth(), getDevice()->getSwapChain()->getSwapChainImageHeight(),
                           1, 1, 1, 1, "ShadowMap"),
                mShadowMapView(mShadowMap, ImageViewType::Colour),
                mRayTracedShadowsShader(eng->getShader("./Shaders/RayTracedShadows.comp"))
{
    ComputeTask task("RayTracedShadows");
    task.addInput(kCameraBuffer, AttachmentType::UniformBuffer);
    task.addInput(kGBufferDepth, AttachmentType::Texture2D);
    task.addInput(kDefaultSampler, AttachmentType::Sampler);
    task.addInput(kShadowMapRaw, AttachmentType::Image2D);
    task.addInput(kShadowMapHistory, AttachmentType::Texture2D);
    task.addInput(kShadowMapCounter, AttachmentType::Image2D);
    task.addInput(kLinearDepth, AttachmentType::Texture2D);
    task.addInput(kPreviousLinearDepth, AttachmentType::Texture2D);
    task.addInput(kGBufferVelocity, AttachmentType::Texture2D);
    task.addInput(kShadowingLights, AttachmentType::UniformBuffer);
    task.addInput(kBVH, AttachmentType::ShaderResourceSet);
    task.addInput("SampleCount", AttachmentType::PushConstants);

    task.setRecordCommandsCallback(
        [this](const RenderGraph& graph, const uint32_t taskIndex, Executor* exec, RenderEngine* eng, const std::vector<const MeshInstance*>&)
        {
            const RenderTask& task = graph.getTask(taskIndex);
            exec->setComputeShader(static_cast<const ComputeTask&>(task), graph, mRayTracedShadowsShader);

            const uint32_t frameIndex = static_cast<uint32_t>(eng->getDevice()->getCurrentSubmissionIndex() % 64ULL);
            exec->insertPushConstant(&frameIndex, sizeof(uint32_t));

            const float threadGroupWidth = eng->getSwapChainImageView()->getImageExtent().width / 4;
            const float threadGroupHeight = eng->getSwapChainImageView()->getImageExtent().height / 4;

            exec->dispatch(std::ceil(threadGroupWidth / 16.0f), std::ceil(threadGroupHeight / 16.0f), 1.0f);
        }
    );

    // Add a depth aware upsample pass for the quater res shadows.
    const float outputWidth = eng->getSwapChainImageView()->getImageExtent().width;
    const float outputHeight = eng->getSwapChainImageView()->getImageExtent().height;
    addDeferredUpsampleTaskR8("upsample shadows", kShadowMapRaw, kShadowMap, uint2(outputWidth, outputHeight), eng, graph);

    mTaskID = graph.addTask(task);
}


void RayTracedShadowsTechnique::bindResources(RenderGraph& graph)
    {
        if(!graph.isResourceSlotBound(kShadowMap))
        {
            graph.bindImage(kShadowMap, mShadowMapView);
            graph.bindImage(kShadowMapCounter, mShadowMapCounterView);
        }

        const uint64_t frameIndex = getDevice()->getCurrentSubmissionIndex();
        if(frameIndex % 2)
        {
            graph.bindImage(kShadowMapRaw, mShadowMapHistoryView[0]);
            graph.bindImage(kShadowMapHistory, mShadowMapHistoryView[1]);
        }
        else
        {
            graph.bindImage(kShadowMapRaw, mShadowMapHistoryView[1]);
            graph.bindImage(kShadowMapHistory, mShadowMapHistoryView[0]);
        }

    }
