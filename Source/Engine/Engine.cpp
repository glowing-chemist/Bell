#include "Core/ConversionUtils.hpp"
#include "Core/CommandContext.hpp"
#include "Core/BellLogging.hpp"
#include "Core/Executor.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanRenderInstance.hpp" 
#endif

#ifdef DX_12
#include "Core/DX_12/DX_12RenderInstance.hpp"
#endif

#include "Engine/Engine.hpp"
#include "Engine/TextureUtil.hpp"
#include "Engine/PreDepthTechnique.hpp"
#include "Engine/GBufferTechnique.hpp"
#include "Engine/GBufferMaterialTechnique.hpp"
#include "Engine/SSAOTechnique.hpp"
#include "Engine/BlurXTechnique.hpp"
#include "Engine/BlurYTechnique.hpp"
#include "Engine/OverlayTechnique.hpp"
#include "Engine/ImageBasedLightingTechnique.hpp"
#include "Engine/DFGGenerationTechnique.hpp"
#include "Engine/ConvolveSkyboxTechnique.hpp"
#include "Engine/SkyboxTechnique.hpp"
#include "Engine/CompositeTechnique.hpp"
#include "Engine/ForwardIBLTechnique.hpp"
#include "Engine/ForwardLightingCombinedTechnique.hpp"
#include "Engine/LightFroxelationTechnique.hpp"
#include "Engine/DeferredAnalyticalLightingTechnique.hpp"
#include "Engine/ShadowMappingTechnique.hpp"
#include "Engine/TAATechnique.hpp"
#include "Engine/LineariseDepthTechnique.hpp"
#include "Engine/ScreenSpaceReflectionTechnique.hpp"
#include "Engine/VoxalizeTechnique.hpp"
#include "Engine/DebugVisualizationTechnique.hpp"
#include "Engine/TransparentTechnique.hpp"
#include "Engine/CascadeShadowMappingTechnique.hpp"
#include "Engine/SkinningTechnique.hpp"
#include "Engine/DeferredProbeGITechnique.hpp"
#include "Engine/VisualizeLightProbesTechnique.hpp"
#include "Engine/OcclusionCullingTechnique.hpp"
#include "Engine/PathTracingTechnique.hpp"

#include "Engine/RayTracedScene.hpp"

#include "glm/gtx/transform.hpp"

#include <numeric>
#include <thread>


Engine::Engine(GLFWwindow* windowPtr) :
    mThreadPool(),
#ifdef VULKAN
    mRenderInstance( new VulkanRenderInstance(windowPtr)),
#endif
#ifdef DX_12
    mRenderInstance(new DX_12RenderInstance(windowPtr)),
#endif
    mRenderDevice(mRenderInstance->createRenderDevice(DeviceFeaturesFlags::Compute | DeviceFeaturesFlags::Subgroup | DeviceFeaturesFlags::Geometry | DeviceFeaturesFlags::RayTracing)),
    mCurrentScene(nullptr),
    mRayTracedScene(nullptr),
    mDebugCameraActive(false),
    mDebugCamera({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 1.0f, 0.1f, 2000.0f),
    mAnimationVertexBuilder(),
    mBoneIndexBuilder(),
    mBoneWeightBuilder(),
    mVertexBuilder(),
    mIndexBuilder(),
    mMaterials{getDevice(), 200},
    mLTCMat(getDevice(), Format::RGBA32Float, ImageUsage::Sampled | ImageUsage::TransferDest, 64, 64, 1, 1, 1, 1, "LTC Mat"),
    mLTCMatView(mLTCMat, ImageViewType::Colour),
    mLTCAmp(getDevice(), Format::RG32Float, ImageUsage::Sampled | ImageUsage::TransferDest, 64, 64, 1, 1, 1, 1, "LTC Amp"),
    mLTCAmpView(mLTCAmp, ImageViewType::Colour),
    mInitialisedTLCTextures(false),
    mCurrentRenderGraph(),
    mCompileGraph(true),
	mTechniques{},
    mPassesRegisteredThisFrame{0},
	mCurrentRegistredPasses{0},
    mShaderPrefix{},
    mVertexBuffer{getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest | BufferUsage::DataBuffer, 10000000, 10000000, "Vertex Buffer"},
    mIndexBuffer{getDevice(), BufferUsage::Index | BufferUsage::TransferDest | BufferUsage::DataBuffer, 10000000, 10000000, "Index Buffer"},
    mTposeVertexBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, 10000000, 10000000, "TPose Vertex Buffer"),
    mBonesWeightsBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(uint2) * 30000, sizeof(uint2) * 30000, "Bone weights"),
    mBoneWeightsIndexBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(uint2) * 30000, sizeof(uint2) * 30000, "Bone weight indicies"),
    mBoneBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(float4x4) * 1000, sizeof(float4x4) * 1000, "Bone buffer"),
    mMeshBoundsBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(float4) * 1000, sizeof(float4) * 1000, "Bounds buffer"),
    mDefaultSampler(SamplerType::Linear),
    mShowDebugTexture(false),
    mDebugTextureName(""),
    mCameraBuffer{},
	mDeviceCameraBuffer{getDevice(), BufferUsage::Uniform, sizeof(CameraBuffer), sizeof(CameraBuffer), "Camera Buffer"},
    mShadowCastingLight(getDevice(), BufferUsage::Uniform, sizeof(Scene::ShadowingLight), sizeof(Scene::ShadowingLight), "ShadowingLight"),
    mLightBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, (sizeof(Scene::Light) * 1000) + sizeof(uint32_t), (sizeof(Scene::Light) * 1000) + sizeof(uint32_t), "LightBuffer"),
    mLightBufferView(mLightBuffer, sizeof(uint4)),
    mLightCountView(mLightBuffer, 0, sizeof(uint4)),
    mLightsSRS(getDevice(), 2),
    mMaxCommandThreads(1),
    mLightProbeResourceSet(mRenderDevice, 3),
    mRecordTasksSync{true},
    mAsyncTaskContextMappings{},
    mWindow(windowPtr)
{
    for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
    {
        mLightsSRS.get(i)->addDataBufferRO(mLightCountView.get(i));
        mLightsSRS.get(i)->addDataBufferRW(mLightBufferView.get(i));
        mLightsSRS.get(i)->finalise();
    }

    // calculate the TAA jitter.
    auto halton_2_3 = [](const uint32_t index) -> float2
    {
        auto halton = [](uint32_t i, const uint32_t base) -> float
        {
            float f = 1.0f;
            float r = 0.0f;

            while (i > 0.0f)
            {
                f = f / float(base);
                r = r + f * (i % base);
                i = float(i) / float(base);
            }

            return r;
        };

        return float2(halton(index, 2), halton(index, 3));
    };
    for (uint32_t i = 0; i < 16; ++i)
    {
        mTAAJitter[i] = (halton_2_3(i) - 0.5f);
    }

    // load default meshes.
    mUnitSphere = std::make_unique<StaticMesh>("./Assets/unitSphere.fbx", VertexAttributes::Position4 | VertexAttributes::Normals);

    // initialize debug camera
    int width, height;
    glfwGetWindowSize(windowPtr, &width, &height);
    mDebugCamera.setAspect(float(width) / float(height));

    mAnimationLastTicked = std::chrono::system_clock::now();
}


void Engine::setScene(const std::string& path)
{
    mCurrentScene = new Scene(path);
    mCurrentScene->loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals | VertexAttributes::Albedo, this);

    mCurrentScene->uploadData(this);
    mCurrentScene->computeBounds(MeshType::Static);
    mCurrentScene->computeBounds(MeshType::Dynamic);

    setScene(mCurrentScene);
}


void Engine::setScene(Scene* scene)
{
    mCurrentScene = scene;

    if(scene)
    {
        // Set up the SRS for the materials.
        const auto& materials = mCurrentScene->getMaterials();
        mMaterials->addSampledImageArray(materials);
        mMaterials->finalise();
    }
    else // We're clearing the scene so need to destroy the materials.
    {
        mMaterials.reset(mRenderDevice, 200);
        mActiveAnimations.clear();
        mVertexCache.clear();
        mTposeVertexCache.clear();
        mMeshBoundsCache.clear();
        // need to invalidate render pipelines as the number of materials could change.
        mRenderDevice->invalidatePipelines();
    }
}


void Engine::setRayTracingScene(RayTracingScene* scene)
{
    mRayTracedScene = scene;
}


Camera& Engine::getCurrentSceneCamera()
{
    return mDebugCameraActive ? mDebugCamera : mCurrentScene ? mCurrentScene->getCamera() : mDebugCamera;
}


Image Engine::createImage(const uint32_t x,
				  const uint32_t y,
				  const uint32_t z,
                  const uint32_t mips,
                  const uint32_t levels,
                  const uint32_t samples,
				  const Format format,
				  ImageUsage usage,
				  const std::string& name)
{
    return Image{mRenderDevice, format, usage, x, y, z, mips, levels, samples, name};
}

Buffer Engine::createBuffer(const uint32_t size,
					const uint32_t stride,
					BufferUsage usage,
					const std::string& name)
{
	return Buffer{mRenderDevice, usage, size, stride, name};
}


Shader Engine::getShader(const std::string& path)
{
    std::hash<std::string> pathHasher{};
    const uint64_t hashedPath = pathHasher(path);
    const uint64_t shaderKey = hashedPath + mCurrentRegistredPasses;
	if(mShaderCache.find(shaderKey) != mShaderCache.end())
		return (*mShaderCache.find(shaderKey)).second;

    Shader newShader{mRenderDevice, path};

	const bool compiled = newShader->compile(mShaderPrefix);

	BELL_ASSERT(compiled, "Shader failed to compile")

	mShaderCache.insert({ shaderKey, newShader});

	return newShader;
}


std::unique_ptr<Technique> Engine::getSingleTechnique(const PassType passType)
{
    switch(passType)
    {
        case PassType::DepthPre:
            return std::make_unique<PreDepthTechnique>(this, mCurrentRenderGraph);

        case PassType::GBuffer:
            return std::make_unique<GBufferTechnique>(this, mCurrentRenderGraph);

        case PassType::GBufferMaterial:
            return std::make_unique<GBufferMaterialTechnique>(this, mCurrentRenderGraph);

        case PassType::SSAO:
            return std::make_unique<SSAOTechnique>(this, mCurrentRenderGraph);

		case PassType::SSAOImproved:
			return std::make_unique<SSAOImprovedTechnique>(this, mCurrentRenderGraph);

        case PassType::GBufferPreDepth:
            return std::make_unique<GBufferPreDepthTechnique>(this, mCurrentRenderGraph);

        case PassType::GBUfferMaterialPreDepth:
            return std::make_unique<GBufferMaterialPreDepthTechnique>(this, mCurrentRenderGraph);

		case PassType::Overlay:
			return std::make_unique<OverlayTechnique>(this, mCurrentRenderGraph);

        case PassType::DeferredPBRIBL:
            return std::make_unique<DeferredImageBasedLightingTechnique>(this, mCurrentRenderGraph);

		case PassType::ConvolveSkybox:
			return std::make_unique<ConvolveSkyBoxTechnique>(this, mCurrentRenderGraph);

		case PassType::Skybox:
			return std::make_unique<SkyboxTechnique>(this, mCurrentRenderGraph);

		case PassType::DFGGeneration:
			return std::make_unique<DFGGenerationTechnique>(this, mCurrentRenderGraph);

		case PassType::Composite:
			return std::make_unique<CompositeTechnique>(this, mCurrentRenderGraph);

		case PassType::ForwardIBL:
			return std::make_unique<ForwardIBLTechnique>(this, mCurrentRenderGraph);

        case PassType::ForwardCombinedLighting:
            return std::make_unique<ForwardCombinedLightingTechnique>(this, mCurrentRenderGraph);

		case PassType::LightFroxelation:
			return std::make_unique<LightFroxelationTechnique>(this, mCurrentRenderGraph);

		case PassType::DeferredAnalyticalLighting:
			return std::make_unique<DeferredAnalyticalLightingTechnique>(this, mCurrentRenderGraph);

        case PassType::Shadow:
            return std::make_unique<ShadowMappingTechnique>(this, mCurrentRenderGraph);

        case PassType::TAA:
            return std::make_unique<TAATechnique>(this, mCurrentRenderGraph);

        case PassType::LineariseDepth:
            return std::make_unique<LineariseDepthTechnique>(this, mCurrentRenderGraph);

        case PassType::SSR:
            return std::make_unique<ScreenSpaceReflectionTechnique>(this, mCurrentRenderGraph);

        case PassType::Voxelize:
            return std::make_unique<VoxalizeTechnique>(this, mCurrentRenderGraph);

        case PassType::DebugAABB:
            return std::make_unique<DebugAABBTechnique>(this, mCurrentRenderGraph);

        case PassType::Transparent:
            return std::make_unique<TransparentTechnique>(this, mCurrentRenderGraph);

        case PassType::CascadingShadow:
            return std::make_unique<CascadeShadowMappingTechnique>(this, mCurrentRenderGraph);

        case PassType::Animation:
            return std::make_unique<SkinningTechnique>(this, mCurrentRenderGraph);

        case PassType::LightProbeDeferredGI:
            return std::make_unique<DeferredProbeGITechnique>(this, mCurrentRenderGraph);

        case PassType::VisualizeLightProbes:
            return std::make_unique<ViaualizeLightProbesTechnique>(this, mCurrentRenderGraph);

        case PassType::OcclusionCulling:
            return std::make_unique<OcclusionCullingTechnique>(this, mCurrentRenderGraph);

        case PassType::PathTracing:
            return std::make_unique<PathTracingTechnique>(this, mCurrentRenderGraph);

        case PassType::RayTracedReflections:
            return std::make_unique<RayTracedReflectionTechnique>(this, mCurrentRenderGraph);

        default:
        {
            BELL_TRAP;
            return nullptr;
        }
    }
}

std::pair<uint64_t, uint64_t> Engine::addMeshToBuffer(const StaticMesh* mesh)
{
	const auto it = mVertexCache.find(mesh);
	if (it != mVertexCache.end())
		return it->second;

    const auto& vertexData = mesh->getVertexData();
    const auto& indexData = mesh->getIndexData();

	const auto vertexOffset = mVertexBuilder.addData(vertexData);

    const auto indexOffset = mIndexBuilder.addData(indexData);

	mVertexCache.insert(std::make_pair( mesh, std::pair<uint64_t, uint64_t>{vertexOffset, indexOffset }));

    return {vertexOffset, indexOffset};
}


std::pair<uint64_t, uint64_t> Engine::addMeshToAnimationBuffer(const StaticMesh* mesh)
{
    const auto it = mTposeVertexCache.find(mesh);
    if(it != mTposeVertexCache.end())
        return it->second;

    const auto& vertexData = mesh->getVertexData();
    const auto vertexOffset = mAnimationVertexBuilder.addData(vertexData);

    const auto& boneIndicies = mesh->getBoneIndicies();
    mBoneIndexBuilder.addData(boneIndicies);

    const auto& boneWeights = mesh->getBoneWeights();
    const auto boneWeightsOffset = mBoneWeightBuilder.addData(boneWeights);

    mTposeVertexCache.insert(std::make_pair( mesh, std::pair<uint64_t, uint64_t>{vertexOffset, boneWeightsOffset}));

    return {vertexOffset, boneWeightsOffset};
}


uint64_t Engine::getMeshBoundsIndex(const MeshInstance* inst)
{
    BELL_ASSERT(mMeshBoundsCache.find(inst) != mMeshBoundsCache.end(), "Unable to fin dinstance bounds")

    return mMeshBoundsCache[inst];
}


void Engine::execute(RenderGraph& graph)
{
    // Upload vertex/index data if required.
    auto& vertexData = mVertexBuilder.finishRecording();
    auto& indexData = mIndexBuilder.finishRecording();
    if(!vertexData.empty() && !indexData.empty())
    {
        mVertexBuffer->resize(static_cast<uint32_t>(vertexData.size()), false);
        mIndexBuffer->resize(static_cast<uint32_t>(indexData.size()), false);

        mVertexBuffer->setContents(vertexData.data(), static_cast<uint32_t>(vertexData.size()));
        mIndexBuffer->setContents(indexData.data(), static_cast<uint32_t>(indexData.size()));

        mVertexBuilder.reset();
        mIndexBuilder.reset();

        // upload mesh instance bounds info.
        static_assert(sizeof(AABB) == (sizeof(float4) * 2), "Sizes need to match");
        std::vector<AABB> bounds{};
        for(const auto& inst : mCurrentScene->getStaticMeshInstances())
        {
            mMeshBoundsCache.insert({&inst, bounds.size()});
            bounds.push_back(inst.mMesh->getAABB() * inst.getTransMatrix());
        }
        for(const auto& inst : mCurrentScene->getDynamicMeshInstances())
        {
            mMeshBoundsCache.insert({&inst, bounds.size()});
            bounds.push_back(inst.mMesh->getAABB() * inst.getTransMatrix());
        }

        mMeshBoundsBuffer->setContents(bounds.data(), sizeof(AABB) * bounds.size());
    }

    auto& animationVerticies = mAnimationVertexBuilder.finishRecording();
    auto& boneWeights = mBoneWeightBuilder.finishRecording();
    auto& boneIndicies = mBoneIndexBuilder.finishRecording();
    if(!animationVerticies.empty() && !boneIndicies.empty())
    {
        mTposeVertexBuffer->resize(static_cast<uint32_t>(animationVerticies.size()), false);
        mBoneWeightsIndexBuffer->resize(static_cast<uint32_t>(boneIndicies.size()), false);
        mBonesWeightsBuffer->resize(static_cast<uint32_t>(boneWeights.size()), false);

        mTposeVertexBuffer->setContents(animationVerticies.data(), static_cast<uint32_t>(animationVerticies.size()));
        mBoneWeightsIndexBuffer->setContents(boneIndicies.data(), static_cast<uint32_t>(boneIndicies.size()));
        mBonesWeightsBuffer->setContents(boneWeights.data(), static_cast<uint32_t>(boneWeights.size()));

        mAnimationVertexBuilder.reset();
        mBoneIndexBuilder.reset();
    }

    // Finalize graph internal state.
    if(mCompileGraph)
    {
        graph.compile(mRenderDevice);

        mCurrentRenderGraph.bindShaderResourceSet(kMaterials, mMaterials);
        mCurrentRenderGraph.bindImage(kLTCMat, mLTCMatView);
        mCurrentRenderGraph.bindImage(kLTCAmp, mLTCAmpView);
        mCurrentRenderGraph.bindSampler(kDefaultSampler, mDefaultSampler);
        mCurrentRenderGraph.bindVertexBuffer(kSceneVertexBuffer, mVertexBuffer);
        mCurrentRenderGraph.bindIndexBuffer(kSceneIndexBuffer, mIndexBuffer);

        mCurrentRenderGraph.bindBuffer(kTPoseVertexBuffer, mTposeVertexBuffer);
        mCurrentRenderGraph.bindBuffer(kBonesWeights, mBonesWeightsBuffer);
        mCurrentRenderGraph.bindBuffer(kBoneWeighntsIndiciesBuffer, mBoneWeightsIndexBuffer);
        mCurrentRenderGraph.bindBuffer(kBonesBuffer, *mBoneBuffer);

        mCurrentRenderGraph.bindBuffer(kMeshBoundsBuffer, mMeshBoundsBuffer);

        if(mCurrentScene && mCurrentScene->getSkybox())
        {
            mCurrentRenderGraph.bindImage(kSkyBox, *mCurrentScene->getSkybox());
            (*mCurrentScene->getSkybox())->updateLastAccessed();
        }

        if(mCurrentScene && mCurrentScene->getSkybox())
            mCurrentRenderGraph.bindImage(kSkyBox, *mCurrentScene->getSkybox());

        if(mIrradianceProbeBuffer)
            mCurrentRenderGraph.bindShaderResourceSet(kLightProbes, mLightProbeResourceSet);

        if(mRayTracedScene)
            mCurrentRenderGraph.bindShaderResourceSet(kBVH, mRayTracedScene->getGPUBVH());

        mCompileGraph = false;
    }

    for(const auto& tech : mTechniques)
    {
        tech->bindResources(mCurrentRenderGraph);
    }

    updateGlobalBuffers();

    mMaterials->updateLastAccessed();
    mCurrentRenderGraph.bindBuffer(kCameraBuffer, *mDeviceCameraBuffer);
    mCurrentRenderGraph.bindBuffer(kShadowingLights, *mShadowCastingLight);
    mCurrentRenderGraph.bindShaderResourceSet(kLightBuffer, *mLightsSRS);

    if(!mActiveAnimations.empty())
    {
        mCurrentRenderGraph.bindBuffer(kBonesBuffer, *mBoneBuffer);
    }

    if(mCurrentScene && mCurrentScene->getSkybox())
        (*mCurrentScene->getSkybox())->updateLastAccessed();

    std::vector<const MeshInstance*> meshes{};

    if(mCurrentScene)
    {
        meshes = mCurrentScene ? mCurrentScene->getViewableMeshes(mCurrentScene->getCamera().getFrustum()) : std::vector<const MeshInstance*>{};
        std::sort(meshes.begin(), meshes.end(), [camera = mCurrentScene->getCamera()](const MeshInstance* lhs, const MeshInstance* rhs)
        {
            const float3 centralLeft = (lhs->mMesh->getAABB() *  lhs->getTransMatrix()).getCentralPoint();
            const float leftDistance = glm::length(centralLeft - camera.getPosition());

            const float3 centralright = (rhs->mMesh->getAABB() * rhs->getTransMatrix()).getCentralPoint();
            const float rightDistance = glm::length(centralright - camera.getPosition());

            return leftDistance < rightDistance;
        });
    }

    //printf("Meshes %zu\n", meshes.size());

    auto barriers = graph.generateBarriers(mRenderDevice);
	
    uint32_t currentContext = 0;
    if (mRecordTasksSync)
    {
        auto barrier = barriers.begin();
        const uint32_t taskCount = graph.taskCount();
        uint32_t recordedCommands = 0;
        mAsyncTaskContextMappings.clear();
        mAsyncTaskContextMappings.push_back({0, 0});
        for (uint32_t taskIndex = 0; taskIndex < taskCount; ++taskIndex)
        {
            ContextMapping& ctxMapping = mAsyncTaskContextMappings[currentContext];
            ++ctxMapping.mTaskCount;

            RenderTask& task = graph.getTask(taskIndex);

            CommandContextBase* context = mRenderDevice->getCommandContext(currentContext);
            Executor* exec = context->allocateExecutor(GPU_PROFILING);
            exec->recordBarriers(*barrier);
            context->setupState(graph, taskIndex, exec, mCurrentRegistredPasses);

            task.executeRecordCommandsCallback(exec, this, meshes);

            recordedCommands += exec->getRecordedCommandCount();
            exec->resetRecordedCommandCount();

            context->freeExecutor(exec);
            ++barrier;

            if (taskIndex < (taskCount - 1) && recordedCommands >= 250) // submit context
            {
                recordedCommands = 0;
                ++currentContext;
                mRenderDevice->submitContext(context);
                mAsyncTaskContextMappings.push_back({taskIndex + 1, 0});
            }
        }

        mRecordTasksSync = false;
    }
    else // Record tasks asynchronously.
    {
        // make sure we have enough contexst.
        mRenderDevice->getCommandContext(mAsyncTaskContextMappings.size() - 1);

        auto recordToContext = [&](const ContextMapping& ctxMapping, const uint32_t contextIndex) -> CommandContextBase*
        {
            // Correct number of contexts will have been created when running tasks sync.
            CommandContextBase* context = mRenderDevice->getCommandContext(contextIndex);

            for (uint32_t taskOffset = 0; taskOffset < ctxMapping.mTaskCount; ++taskOffset)
            {
                const uint32_t taskIndex = ctxMapping.mTaskStartIndex + taskOffset;

                Executor* exec = context->allocateExecutor(GPU_PROFILING);
                exec->recordBarriers(barriers[taskIndex]);
                context->setupState(graph, taskIndex, exec, mCurrentRegistredPasses);

                RenderTask& task = graph.getTask(taskIndex);
                task.executeRecordCommandsCallback(exec, this, meshes);

                exec->resetRecordedCommandCount();

                context->freeExecutor(exec);
            }

            return context;
        };

        std::vector<std::future<CommandContextBase*>> resultHandles{};
        resultHandles.reserve(mAsyncTaskContextMappings.size());
        for (uint32_t i = 1; i < mAsyncTaskContextMappings.size(); ++i)
        {
            resultHandles.push_back(mThreadPool.addTask(recordToContext, mAsyncTaskContextMappings[i], i));
        }

        CommandContextBase* firstCtx = recordToContext(mAsyncTaskContextMappings[0], 0);
        if(mAsyncTaskContextMappings.size() > 1) // If this isn't the first and last context submit it now, if not we still need to record the frame buffer transition.
            mRenderDevice->submitContext(firstCtx);

        for (uint32_t i = 0; i < resultHandles.size(); ++i)
        {
            std::future<CommandContextBase*>& result = resultHandles[i];

            CommandContextBase* context = result.get();

            if(&result != &resultHandles.back())
                mRenderDevice->submitContext(context);
        }

        currentContext = mAsyncTaskContextMappings.size() - 1;
    }


    CommandContextBase* context = mRenderDevice->getCommandContext(currentContext);
    Executor* exec = context->allocateExecutor();
	// Transition the swapchain image to a presentable format.
	BarrierRecorder frameBufferTransition{ mRenderDevice };
	auto& frameBufferView = getSwapChainImageView();
	frameBufferTransition->transitionLayout(frameBufferView, ImageLayout::Present, Hazard::ReadAfterWrite, SyncPoint::FragmentShaderOutput, SyncPoint::BottomOfPipe);

    exec->recordBarriers(frameBufferTransition);
    context->freeExecutor(exec);

    mRenderDevice->submitContext(context, true);
}


void Engine::startAnimation(const InstanceID id, const std::string& name, const bool loop, const float speedModifer)
{
    mActiveAnimations.push_back({name, id, speedModifer, 0, 0.0, loop});
}


void Engine::terimateAnimation(const InstanceID id, const std::string& name)
{
    mActiveAnimations.erase(std::remove_if(mActiveAnimations.begin(), mActiveAnimations.end(), [id, name](const AnimationEntry& entry)
    {
        return entry.mName == name && entry.mMesh == id;
    }));
}


void Engine::tickAnimations()
{
    const std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    const std::chrono::nanoseconds elapsed = currentTime - mAnimationLastTicked;
    double elapsedTime = elapsed.count();
    elapsedTime /= 1000000000.0;
    mAnimationLastTicked = currentTime;
    uint64_t boneOffset = 0;
    std::vector<float4x4> boneMatracies{};

    for(auto& animEntry : mActiveAnimations)
    {
        MeshInstance* instance = mCurrentScene->getMeshInstance(animEntry.mMesh);
        Animation& animation = instance->mMesh->getAnimation(animEntry.mName);
        double ticksPerSec = animation.getTicksPerSec();
        animEntry.mTick += elapsedTime * ticksPerSec * animEntry.mSpeedModifier;
        animEntry.mBoneOffset = boneOffset;

        if((animation.getTotalTicks() - animEntry.mTick) > 0.00001)
        {
            std::vector<float4x4> matracies  = animation.calculateBoneMatracies(*instance->mMesh, animEntry.mTick);
            boneOffset += matracies.size();
            boneMatracies.insert(boneMatracies.end(), matracies.begin(), matracies.end());
        }
        else if(animEntry.mLoop)
            animEntry.mTick = 0.0;
    }

    if(!boneMatracies.empty())
        (*mBoneBuffer)->setContents(boneMatracies.data(), boneMatracies.size() * sizeof(float4x4));

    mActiveAnimations.erase(std::remove_if(mActiveAnimations.begin(), mActiveAnimations.end(), [this](const AnimationEntry& entry)
    {
        Animation& animation = mCurrentScene->getAnimation(entry.mMesh, entry.mName);
        return (entry.mTick - animation.getTotalTicks()) > 0.00001;
    }), mActiveAnimations.end());
}


void Engine::recordScene()
{   
    // Add new techniques
    if (mPassesRegisteredThisFrame > 0)
    {
        mCurrentRegistredPasses |= mPassesRegisteredThisFrame;

        while (mPassesRegisteredThisFrame > 0)
        {
            static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "builtin requires sizes be the same ");
            const uint64_t passTypeIndex = __builtin_ctzll(mPassesRegisteredThisFrame);

            mTechniques.push_back(getSingleTechnique(static_cast<PassType>(1ull << passTypeIndex)));
            mPassesRegisteredThisFrame &= mPassesRegisteredThisFrame - 1;
        }
    }

	BELL_ASSERT(!mTechniques.empty(), "Need at least one technique registered with the engine")

	for(auto& tech : mTechniques)
	{
		tech->render(mCurrentRenderGraph, this);
	}
}


void Engine::render()
{
	execute(mCurrentRenderGraph);
}


void Engine::updateGlobalBuffers()
{
    if(!mInitialisedTLCTextures)
    {
        // Load textures for LTC.
        TextureUtil::TextureInfo matInfo = TextureUtil::load128BitTexture("./Assets/ltc_mat.hdr", 4);
        mLTCMat->setContents(matInfo.mData.data(), matInfo.width, matInfo.height, 1);

        TextureUtil::TextureInfo ampInfo = TextureUtil::load128BitTexture("./Assets/ltc_amp.hdr", 2);
        mLTCAmp->setContents(ampInfo.mData.data(), ampInfo.width, ampInfo.height, 1);

        mInitialisedTLCTextures = true;
    }

    // Update camera UB.
    if(mCurrentScene)
    {
        {
            Camera& currentCamera = mDebugCameraActive ? mDebugCamera : getCurrentSceneCamera();

            const float swapchainX = getSwapChainImage()->getExtent(0, 0).width;
            const float swapChainY = getSwapChainImage()->getExtent(0, 0).height;

            float4x4 view = currentCamera.getViewMatrix();
            float4x4 perspective;
            mCameraBuffer.mPreviousFrameViewProjMatrix = mCameraBuffer.mViewProjMatrix;
            // need to add jitter for TAA
            if(isPassRegistered(PassType::TAA))
            {
                const uint32_t index = mRenderDevice->getCurrentSubmissionIndex() % 16;
                const float2& jitter = mTAAJitter[index];

                perspective = glm::translate(float3(jitter / mCameraBuffer.mFrameBufferSize, 0.0f)) * currentCamera.getPerspectiveMatrix();
                mCameraBuffer.mPreviousJitter = mCameraBuffer.mJitter;
                mCameraBuffer.mJitter = jitter / mCameraBuffer.mFrameBufferSize;
            }
            else
            {
                perspective = currentCamera.getPerspectiveMatrix();
                mCameraBuffer.mPreviousJitter = mCameraBuffer.mJitter;
                mCameraBuffer.mJitter = float2(0.0f, 0.0f);
            }
            mCameraBuffer.mViewProjMatrix = perspective * view;
            mCameraBuffer.mInvertedViewProjMatrix = glm::inverse(mCameraBuffer.mViewProjMatrix);
            mCameraBuffer.mInvertedPerspective = glm::inverse(perspective);
            mCameraBuffer.mViewMatrix = view;
            mCameraBuffer.mPerspectiveMatrix = perspective;
            mCameraBuffer.mOrthoMatrix = currentCamera.getOrthographicsMatrix();
            mCameraBuffer.mFrameBufferSize = glm::vec2{ swapchainX, swapChainY };
            mCameraBuffer.mPosition = currentCamera.getPosition();
            mCameraBuffer.mNeaPlane = currentCamera.getNearPlane();
            mCameraBuffer.mFarPlane = currentCamera.getFarPlane();
            mCameraBuffer.mFOV = currentCamera.getFOV();
            mCameraBuffer.mSceneSize = float4(mCurrentScene->getBounds().getSideLengths(), 1.0f);

            MapInfo mapInfo{};
            mapInfo.mSize = sizeof(CameraBuffer);
            mapInfo.mOffset = 0;

            void* cameraBufferPtr = mDeviceCameraBuffer.get()->map(mapInfo);

            std::memcpy(cameraBufferPtr, &mCameraBuffer, sizeof(CameraBuffer));

            mDeviceCameraBuffer.get()->unmap();
        }

        {
            mLightBuffer.get()->setContents(static_cast<int>(mCurrentScene->getLights().size()), sizeof(uint32_t));

            if(!mCurrentScene->getLights().empty())
                mLightBuffer.get()->setContents(mCurrentScene->getLights().data(), static_cast<uint32_t>(mCurrentScene->getLights().size() * sizeof(Scene::Light)), sizeof(uint4));
        }

        {
            const Scene::ShadowingLight& shadowingLight = mCurrentScene->getShadowingLight();

            MapInfo mapInfo{};
            mapInfo.mSize = sizeof(Scene::ShadowingLight);
            mapInfo.mOffset = 0;

            void* dst = mShadowCastingLight.get()->map(mapInfo);

                std::memcpy(dst, &shadowingLight, sizeof(Scene::ShadowingLight));

            mShadowCastingLight.get()->unmap();
        }

        if(!mActiveAnimations.empty())
        {
            tickAnimations();
        }
        else
            mAnimationLastTicked = std::chrono::system_clock::now();
    }
}


void Engine::registerPass(const PassType pass)
{
	if((static_cast<uint64_t>(pass) & mCurrentRegistredPasses) == 0)
	{
        mShaderPrefix.push_back(std::string(passToString(pass)));

        mPassesRegisteredThisFrame |= static_cast<uint64_t>(pass);
	}
}


bool Engine::isPassRegistered(const PassType pass) const
{
	return (static_cast<uint64_t>(pass) & mCurrentRegistredPasses) > 0;
}


void Engine::rayTraceScene()
{
    if(mRayTracedScene)
    {
        const ImageExtent extent = getSwapChainImage()->getExtent(0, 0);
        mRayTracedScene->renderSceneToFile(mCurrentScene->getCamera(), extent.width, extent.height, "./RTNormals.jpg", mThreadPool);
    }
    else
    {
        BELL_LOG("No scene currently active")
    }
}


CPUImage Engine::renderDiffuseCubeMap(const RayTracingScene& scene, const float3 &position, const uint32_t x, const uint32_t y)
{
    std::vector<unsigned char> data(x * y * 6 * sizeof(uint32_t));

    // render +x
    Camera cam{position, float3{1.0f, 0.0f, 0.0f}, 1.0f, 0.1f, 2000.0f};
    scene.renderSceneToMemory(cam, x, y, data.data(), mThreadPool);

    // render -x
    cam.setDirection(float3(-1.0f, 0.0f, 0.0f));
    scene.renderSceneToMemory(cam, x, y, data.data() + (x * y * sizeof(uint32_t)), mThreadPool);

    // render +y
    cam.setDirection(float3(0.0f, 1.0f, 0.0f));
    scene.renderSceneToMemory(cam, x, y, data.data() + (x * y * 2 * sizeof(uint32_t)), mThreadPool);

    // render -y
    cam.setDirection(float3(0.0f, -1.0f, 0.0f));
    scene.renderSceneToMemory(cam, x, y, data.data() + (x * y * 3 * sizeof(uint32_t)), mThreadPool);

    // render +z
    cam.setDirection(float3(0.0f, 0.0f, 1.0f));
    scene.renderSceneToMemory(cam, x, y, data.data() + (x * y * 4 * sizeof(uint32_t)), mThreadPool);

    // render -z
    cam.setDirection(float3(0.0f, 0.0f, -1.0f));
    scene.renderSceneToMemory(cam, x, y, data.data() + (x * y * 5 * sizeof(uint32_t)), mThreadPool);


    return CPUImage(std::move(data), ImageExtent{x, y, 6}, Format::RGBA8UNorm);
}


std::vector<Engine::SphericalHarmonic> Engine::generateIrradianceProbes(const std::vector<IrradianceProbeVolume>& volumes)
{
    std::vector<float3> positions{};
    for(const auto& volume : volumes)
    {
        const auto probePositions = volume.getProbePositions();
        positions.insert(positions.end(), probePositions.begin(), probePositions.end());
    }

    return generateIrradianceProbes(positions);
}


std::vector<Engine::SphericalHarmonic> Engine::generateIrradianceProbes(const std::vector<float3>& positions)
{
    std::vector<Engine::SphericalHarmonic> harmonics{};

    if(mRayTracedScene)
    {
        for(const float3& position : positions)
        {
            const CPUImage cubeMap = renderDiffuseCubeMap(*mRayTracedScene, position, 512, 512);
            const SphericalHarmonic harmonic = generateSphericalHarmonic(position, cubeMap);

            harmonics.push_back(harmonic);
        }
    }

    if(!harmonics.empty())
    {
        mIrradianceProbeBuffer = std::make_unique<Buffer>(mRenderDevice, BufferUsage::TransferDest | BufferUsage::DataBuffer,
                                                          sizeof(Engine::SphericalHarmonic) * harmonics.size(), sizeof(Engine::SphericalHarmonic) * harmonics.size(),
                                                          "Irradiance harmonics");
        mIrradianceProbeBufferView = std::make_unique<BufferView>(*mIrradianceProbeBuffer);

        (*mIrradianceProbeBuffer)->setContents(harmonics.data(), sizeof(Engine::SphericalHarmonic) * harmonics.size());
    }

    return harmonics;
}


Engine::SphericalHarmonic Engine::generateSphericalHarmonic(const float3& position, const CPUImage& cubemap)
{
    SphericalHarmonic harmonic{};
    harmonic.mPosition = float4(position, 1.0f);

    auto getNormalFromTexel = [](const int3 texelPosition, const float2 size)
    {
        float2 position = float2(texelPosition) / size;

        position -= 0.5; // remap 0-1 to -0.5-0.5
        position *= 2; // remap to -1-1

        const uint32_t cubemapSide = texelPosition.z;

        float3 normal;

        if(cubemapSide == 0)
        { // facing x-dir
            normal = float3(1, -position.y, -position.x);
        }
        else if(cubemapSide == 1)
        { // facing -x-dir
            normal = float3(-1, -position.y, position.x);
        }
        else if(cubemapSide == 2)
        { // facing y-dir
            normal = float3(position.x, 1, position.y);
        }
        else if(cubemapSide == 3)
        { // facing -y-dir
            normal = float3(-position.x, -1, position.y);
        }
        else if(cubemapSide == 4)
        { // facing z-dir
            normal = float3(position.x, -position.y, 1);
        }
        else if(cubemapSide == 5)
        { // facing -z-dir
            normal = float3(-position.x, -position.y, -1);
        }
        return glm::normalize(normal);
    };

    auto differentialSolidAngle = [&]( const float a_U, const float a_V, const uint32_t a_Size) -> float
    {
        const float u = a_U / float(a_Size);
        const float v = a_V / float(a_Size);

        float fTmp = 1.0f + (u * u) + (v * v);
        return 4.0f / (sqrt(fTmp)*fTmp);
    };

    float3 Y00 = float3(0.0f);
    float3 Y11 = float3(0.0f);
    float3 Y10 = float3(0.0f);
    float3 Y1_1 = float3(0.0f);
    float3 Y21 = float3(0.0f);
    float3 Y2_1 = float3(0.0f);
    float3 Y2_2 = float3(0.0f);
    float3 Y20 = float3(0.0f);
    float3 Y22 = float3(0.0f);

    const ImageExtent& extent = cubemap.getExtent();
    for(uint32_t faceIndex = 0; faceIndex < 6; ++faceIndex)
    {
        for(uint32_t x = 0; x < extent.width; ++x)
        {
            for(uint32_t y = 0; y < extent.height; ++y)
            {
                const float3 normal = getNormalFromTexel(int3(x, y, faceIndex), float2(extent.width, extent.height));
                const float solidAngle = differentialSolidAngle(float(x), float(y), extent.width);

                const float3 L = cubemap.sampleCube4(normal);

                // Constants map to SH basis functions constants.
                Y00   += solidAngle * L * 0.282095f;
                Y11   += solidAngle * L * 0.488603f * normal.x;
                Y10   += solidAngle * L * 0.488603f * normal.y;
                Y1_1  += solidAngle * L * 0.488603f * normal.z;
                Y21   += solidAngle * L * 1.092548f * (normal.x * normal.z);
                Y2_1  += solidAngle * L * 1.092548f * (normal.y * normal.z);
                Y2_2  += solidAngle * L * 1.092548f * (normal.x * normal.y);
                Y20   += solidAngle * L * 0.315392f * ((3.0f * normal.z * normal.z) - 1.0f);
                Y22   += solidAngle * L * 0.546274f * ((normal.x * normal.x) - (normal.y * normal.y));

            }
        }
    }

    const float sampleCount = 1.0f / float(extent.width * extent.height * 6.0f);
    Y00  *= sampleCount;
    Y11  *= sampleCount;
    Y10  *= sampleCount;
    Y1_1 *= sampleCount;
    Y21  *= sampleCount;
    Y2_1 *= sampleCount;
    Y2_2 *= sampleCount;
    Y20  *= sampleCount;
    Y22  *= sampleCount;

    memcpy(&harmonic.mCoefs[0], &Y00, sizeof(float3));
    memcpy(&harmonic.mCoefs[3], &Y11, sizeof(float3));
    memcpy(&harmonic.mCoefs[6], &Y10, sizeof(float3));
    memcpy(&harmonic.mCoefs[9], &Y1_1, sizeof(float3));
    memcpy(&harmonic.mCoefs[12], &Y21, sizeof(float3));
    memcpy(&harmonic.mCoefs[15], &Y2_1, sizeof(float3));
    memcpy(&harmonic.mCoefs[18], &Y2_2, sizeof(float3));
    memcpy(&harmonic.mCoefs[21], &Y20, sizeof(float3));
    memcpy(&harmonic.mCoefs[24], &Y22, sizeof(float3));

    return harmonic;
}


std::vector<short4> Engine::generateVoronoiLookupTexture(std::vector<SphericalHarmonic>& harmonics, const uint3& textureSize)
{
    std::vector<short4> textureData{};
    textureData.resize(textureSize.x * textureSize.y * textureSize.z);

    const AABB& sceneBounds = mCurrentScene->getBounds();
    const float3 sceneSize = sceneBounds.getSideLengths();
    const float4& sceneStart = sceneBounds.getBottom();

    auto calculateVoronoiIndices = [&](const uint3& pos, std::vector<uint32_t>& indicies) -> short4
    {
        int32_t foundIndicies[3] = {-1, -1, -1};
        uint8_t indiciesFound = 0;

        const float4 scenePos = ((float4(float3(pos), 1.0f) / float4(float3(textureSize), 1.0f)) * float4(sceneSize, 1.0f)) + sceneStart;

        // Loop through all probes and find closest 3 that are visible from position.
        uint32_t indexToCheck = 0;
        while(indexToCheck < harmonics.size())
        {
            // Find closest probe.
            std::nth_element(indicies.begin(), indicies.begin() + indexToCheck, indicies.end(), [&](const uint32_t lhs, const uint32_t rhs)
            {
                return glm::distance(scenePos, harmonics[lhs].mPosition) < glm::distance(scenePos, harmonics[rhs].mPosition);
            });

            float3 probePosition = harmonics[indicies[indexToCheck]].mPosition;

            const bool visible = mRayTracedScene->isVisibleFrom(scenePos, probePosition);
            if(visible)
            {
                foundIndicies[indiciesFound++] = indicies[indexToCheck];
            }

            ++indexToCheck;

            if(indiciesFound == 3)
                break;
        }

        BELL_ASSERT(indiciesFound == 3, "Unable to find enough visible light probes from position") // Non fatal but should investiagate probe placement if hit.

        short4 result{};
        result.x = foundIndicies[0];
        result.y = foundIndicies[1];
        result.z = foundIndicies[2];
        result.w = 0;

        return result;
    };


    auto calculatePixels = [&](const uint32_t start, const uint32_t stepSize)
    {
        std::vector<uint32_t> indicies{};
        indicies.resize(harmonics.size());
        std::iota(indicies.begin(), indicies.end(), 0); // create initial indicies.

        uint32_t location = start;
        while(location < (textureSize.x * textureSize.y * textureSize.z))
        {
            const uint32_t x = location % textureSize.x;
            const uint32_t y = (location / textureSize.x) % textureSize.y;
            const uint32_t z = location / (textureSize.x * textureSize.y);

            textureData[location] = calculateVoronoiIndices(uint3(x, y, z), indicies);

            location += stepSize;
        }
    };

    std::vector<std::future<void>> handles{};
    const uint32_t threadCount = mThreadPool.getWorkerCount();
    for(uint32_t i = 1; i < threadCount; ++i)
    {
        handles.push_back(mThreadPool.addTask(calculatePixels, i, threadCount));
    }

    calculatePixels(0, threadCount);

    for(auto& thread : handles)
        thread.wait();

    // rebaking to need to flush and reset.
    if(mIrradianceProbeBuffer)
    {
        mRenderDevice->flushWait();

        mLightProbeResourceSet.reset(mRenderDevice, 2);
    }

    mIrradianceVoronoiIrradianceLookup = std::make_unique<Image>(mRenderDevice, Format::RGBA16Int, ImageUsage::TransferDest | ImageUsage::Sampled,
                                                                 textureSize.x, textureSize.y, textureSize.z, 1, 1, 1, "Voronoi probe lookup");
    mIrradianceVoronoiIrradianceLookupView = std::make_unique<ImageView>(*mIrradianceVoronoiIrradianceLookup, ImageViewType::Colour);

    (*mIrradianceVoronoiIrradianceLookup)->setContents(textureData.data(), textureSize.x, textureSize.y, textureSize.z);

    // Set up light probe SRS.
    mLightProbeResourceSet->addDataBufferRO(*mIrradianceProbeBufferView);
    mLightProbeResourceSet->addSampledImage(*mIrradianceVoronoiIrradianceLookupView);
    mLightProbeResourceSet->finalise();

    mIrradianceProbesHarmonics = std::move(harmonics);

    return textureData;
}


void Engine::loadIrradianceProbes(const std::string& probesPath, const std::string& lookupPath)
{
    FILE* probeFile = fopen(probesPath.c_str(), "rb");
    BELL_ASSERT(probeFile, "Unable to find irradiance probe file")
    fseek(probeFile, 0L, SEEK_END);
    const uint32_t probeSize = ftell(probeFile);
    fseek(probeFile, 0L, SEEK_SET);

    BELL_ASSERT(probeSize % sizeof(SphericalHarmonic) == 0, "Invalid sphereical harmonics cache size")
    mIrradianceProbesHarmonics.resize(probeSize / sizeof(SphericalHarmonic));
    fread(mIrradianceProbesHarmonics.data(), sizeof(SphericalHarmonic), mIrradianceProbesHarmonics.size(), probeFile);
    fclose(probeFile);

    mIrradianceProbeBuffer = std::make_unique<Buffer>(mRenderDevice, BufferUsage::TransferDest | BufferUsage::DataBuffer,
                                                      sizeof(Engine::SphericalHarmonic) * mIrradianceProbesHarmonics.size(), sizeof(Engine::SphericalHarmonic) * mIrradianceProbesHarmonics.size(),
                                                      "Irradiance harmonics");
    mIrradianceProbeBufferView = std::make_unique<BufferView>(*mIrradianceProbeBuffer);

    (*mIrradianceProbeBuffer)->setContents(mIrradianceProbesHarmonics.data(), sizeof(Engine::SphericalHarmonic) * mIrradianceProbesHarmonics.size());

    // load lookup texture.
    FILE* lookupFile = fopen(lookupPath.c_str(), "rb");
    BELL_ASSERT(lookupFile, "Unable to find irradiance probe file")
    fseek(lookupFile, 0L, SEEK_END);
    const uint32_t lookupSize = ftell(lookupFile);
    fseek(lookupFile, 0L, SEEK_SET);

    BELL_ASSERT((lookupSize % sizeof(short4)) == 0, "Invalid irradiance probe lookup texture cache size")
    std::vector<short4> textureData{};
    textureData.resize(lookupSize / sizeof(short4));
    fread(textureData.data(), sizeof(short4), textureData.size(), lookupFile);
    fclose(lookupFile);

    if(mIrradianceVoronoiIrradianceLookup)
    {
        mRenderDevice->flushWait();

        mLightProbeResourceSet.reset(mRenderDevice, 2);
    }

    mIrradianceVoronoiIrradianceLookup = std::make_unique<Image>(mRenderDevice, Format::RGBA16Int, ImageUsage::TransferDest | ImageUsage::Sampled,
                                                                 64, 64, 64, 1, 1, 1, "Voronoi probe lookup");
    mIrradianceVoronoiIrradianceLookupView = std::make_unique<ImageView>(*mIrradianceVoronoiIrradianceLookup, ImageViewType::Colour);

    (*mIrradianceVoronoiIrradianceLookup)->setContents(textureData.data(), 64, 64, 64);

    // Set up light probe SRS.
    mLightProbeResourceSet->addDataBufferRO(*mIrradianceProbeBufferView);
    mLightProbeResourceSet->addSampledImage(*mIrradianceVoronoiIrradianceLookupView);
    mLightProbeResourceSet->finalise();
}


Technique* Engine::getRegisteredTechnique(const PassType pass)
{
    if(mCurrentRegistredPasses & static_cast<uint64_t>(pass))
    {
        const auto& technique = std::find_if(mTechniques.begin(), mTechniques.end(), [=](const auto& t)
        {
            return t->getPassType() == pass;
        });
        BELL_ASSERT(technique != mTechniques.end(), "Technique not registered")

        return (*technique).get();
    }
    else
        return nullptr;
}



std::vector<float3> Engine::IrradianceProbeVolume::getProbePositions() const
{
    std::vector<float3> positions{};

    const Basis& volumeBasis = mBoundingBox.getBasisVectors();

    const float3& start = mBoundingBox.getStart();
    float3 endx = (mBoundingBox.getSideLengths().x * volumeBasis.mX);
    float3 endy = (mBoundingBox.getSideLengths().y * volumeBasis.mY);
    float3 endz = (mBoundingBox.getSideLengths().z * volumeBasis.mZ);

    for(float3 startx = float3(0.0f, 0.0f, 0.0f); glm::length(startx) < glm::length(endx); startx += mProbeDensity.x * volumeBasis.mX)
    {
        for(float3 starty = float3(0.0f, 0.0f, 0.0f); glm::length(starty) < glm::length(endy); starty += mProbeDensity.y * volumeBasis.mY)
        {
            for(float3 startz = float3(0.0f, 0.0f, 0.0f); glm::length(startz) < glm::length(endz); startz += mProbeDensity.z * volumeBasis.mZ)
            {
                positions.push_back(start + startx + starty + startz);
            }
        }
    }

    return positions;
}
