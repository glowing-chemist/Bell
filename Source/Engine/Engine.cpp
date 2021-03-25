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
#include "Engine/DownSampleColourTechnique.hpp"
#include "Engine/VoxelTerrainTechnique.hpp"
#include "Engine/InstanceIDTechnique.hpp"

#include "Engine/RayTracedScene.hpp"

#include "glm/gtx/transform.hpp"

#include <numeric>
#include <thread>


RenderEngine::RenderEngine(GLFWwindow* windowPtr) :
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
    mAnimationVertexSize{0},
    mAnimationVertexBuilder(),
    mBoneIndexSize{0},
    mBoneIndexBuilder(),
    mBoneWeightSize{0},
    mBoneWeightBuilder(),
    mVertexSize{0},
    mVertexBuilder(),
    mIndexSize{0},
    mIndexBuilder(),
    mMaterials{getDevice(), 200},
    mLTCMat(getDevice(), Format::RGBA32Float, ImageUsage::Sampled | ImageUsage::TransferDest, 64, 64, 1, 1, 1, 1, "LTC Mat"),
    mLTCMatView(mLTCMat, ImageViewType::Colour),
    mLTCAmp(getDevice(), Format::RG32Float, ImageUsage::Sampled | ImageUsage::TransferDest, 64, 64, 1, 1, 1, 1, "LTC Amp"),
    mLTCAmpView(mLTCAmp, ImageViewType::Colour),
    mInitialisedTLCTextures(false),
    mBlueNoise(getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, 470, 470, 1, 1, 1, 1, "Blue Noise"),
    mBlueNoiseView(mBlueNoise, ImageViewType::Colour),
    mDefaultDiffuseTexture(getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, 4, 4, 1, 1, 1, 1, "Default Diffuse"),
    mDefaultDiffuseView(mDefaultDiffuseTexture, ImageViewType::Colour),
    mCurrentRenderGraph(),
    mCompileGraph(true),
	mTechniques{},
    mPassesRegisteredThisFrame{0},
	mCurrentRegistredPasses{0},
    mShaderPrefix{},
    mVertexBuffer{getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest | BufferUsage::DataBuffer, 10000000, 10000000, "Vertex Buffer"},
    mIndexBuffer{getDevice(), BufferUsage::Index | BufferUsage::TransferDest | BufferUsage::DataBuffer, 100000000, 100000000, "Index Buffer"},
    mTposeVertexBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, 10000000, 10000000, "TPose Vertex Buffer"),
    mBonesWeightsBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(uint2) * 30000, sizeof(uint2) * 30000, "Bone weights"),
    mBoneWeightsIndexBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(uint2) * 30000, sizeof(uint2) * 30000, "Bone weight indicies"),
    mBoneBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(float4x4) * 1000, sizeof(float4x4) * 1000, "Bone buffer"),
    mMeshBoundsBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, sizeof(float4) * 1000, sizeof(float4) * 1000, "Bounds buffer"),
    mDefaultSampler(SamplerType::Linear),
    mDefaultPointSampler(SamplerType::Point),
    mTerrainEnable(false),
    mShowDebugTexture(false),
    mDebugTextureName(""),
    mCameraBuffer{},
	mDeviceCameraBuffer{getDevice(), BufferUsage::Uniform, sizeof(CameraBuffer), sizeof(CameraBuffer), "Camera Buffer"},
    mShadowCastingLight(getDevice(), BufferUsage::Uniform, sizeof(Scene::ShadowingLight), sizeof(Scene::ShadowingLight), "ShadowingLight"),
    mAccumilatedFrameUpdates(0),
    mMaxCommandThreads(1),
    mLightProbeResourceSet(mRenderDevice, 3),
    mRecordTasksSync{true},
    mAsyncTaskContextMappings{},
    mWindow(windowPtr)
{
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
}


void RenderEngine::setScene(const std::string& path)
{
    mCurrentScene = new Scene(path);
    mCurrentScene->loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals | VertexAttributes::Albedo, this);

    mCurrentScene->uploadData(this);
    mCurrentScene->computeBounds(AccelerationStructure::StaticMesh);
    mCurrentScene->computeBounds(AccelerationStructure::DynamicMesh);
    mCurrentScene->computeBounds(AccelerationStructure::Lights);

    setScene(mCurrentScene);
}


void RenderEngine::setScene(Scene* scene)
{
    mCurrentScene = scene;

    if(scene)
    {
        // Set up the SRS for the materials.
        auto& materials = mCurrentScene->getMaterials();
        const uint32_t offset = materials.size();
        materials.push_back(mDefaultDiffuseView);
        mMaterials->addSampledImageArray(materials);
        mMaterials->finalise();

        std::vector<Scene::Material>& materialDescs = mCurrentScene->getMaterialDescriptions();
        materialDescs.push_back(Scene::Material{"Default material", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, offset, static_cast<uint32_t>(MaterialType::Diffuse)});
    }
    else // We're clearing the scene so need to destroy the materials.
    {
        mMaterials.reset(mRenderDevice, 200);
        mActiveSkeletalAnimations.clear();
        mVertexCache.clear();
        mTposeVertexCache.clear();
        mMeshBoundsCache.clear();
        // need to invalidate render pipelines as the number of materials could change.
        mRenderDevice->invalidatePipelines();
    }
}


void RenderEngine::setRayTracingScene(RayTracingScene* scene)
{
    mRayTracedScene = scene;
}


Camera& RenderEngine::getCurrentSceneCamera()
{
    return mDebugCameraActive ? mDebugCamera : mCurrentScene ? mCurrentScene->getCamera() : mDebugCamera;
}


Image RenderEngine::createImage(const uint32_t x,
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

Buffer RenderEngine::createBuffer(const uint32_t size,
                                  const uint32_t stride,
                                  BufferUsage usage,
                                  const std::string& name)
{
	return Buffer{mRenderDevice, usage, size, stride, name};
}


Shader RenderEngine::getShader(const std::string& path)
{
    std::hash<std::string> pathHasher{};
    const uint64_t hashedPath = pathHasher(path);
    const uint64_t shaderKey = hashedPath + mCurrentRegistredPasses;

    std::shared_lock<std::shared_mutex> readLock{ mShaderCacheMutex };

	if(mShaderCache.find(shaderKey) != mShaderCache.end())
    {
        BELL_ASSERT(path.find(((mShaderCache.find(shaderKey)->second))->getFilePath()) != std::string::npos, "Possible has collision?")
		return (*mShaderCache.find(shaderKey)).second;
    }

    // Need exclusive write access.
    readLock.unlock();

    std::unique_lock<std::shared_mutex> writeLock{ mShaderCacheMutex };

    Shader newShader{mRenderDevice, path, mCurrentRegistredPasses};

	const bool compiled = newShader->compile(mShaderPrefix);

	BELL_ASSERT(compiled, "Shader failed to compile")

    BELL_ASSERT(mShaderCache.find(shaderKey) == mShaderCache.end(), "threading issue")
	mShaderCache.insert({ shaderKey, newShader});

	return newShader;
}


Shader RenderEngine::getShader(const std::string& path, const ShaderDefine& define)
{
    std::hash<std::string> pathHasher{};
    uint64_t hashed = pathHasher(path);
    hashed += pathHasher(define.getName());
    hashed += pathHasher(define.getValue());
    const uint64_t shaderKey = hashed + mCurrentRegistredPasses;
    
    std::shared_lock<std::shared_mutex> readLock{ mShaderCacheMutex };

    if(mShaderCache.find(shaderKey) != mShaderCache.end())
        return (*mShaderCache.find(shaderKey)).second;

    readLock.unlock();

    std::unique_lock<std::shared_mutex> writeLock{ mShaderCacheMutex };

    Shader newShader{mRenderDevice, path, mCurrentRegistredPasses};

    std::vector<ShaderDefine> allDefines = mShaderPrefix;
    allDefines.push_back(define);
    const bool compiled = newShader->compile(allDefines);

    BELL_ASSERT(compiled, "Shader failed to compile")

    BELL_ASSERT(mShaderCache.find(shaderKey) == mShaderCache.end(), "threading issue")
    mShaderCache.insert({ shaderKey, newShader});

    return newShader;
}


std::unique_ptr<Technique> RenderEngine::getSingleTechnique(const PassType passType)
{
    switch(passType)
    {
        case PassType::DepthPre:
            return std::make_unique<PreDepthTechnique>(this, mCurrentRenderGraph);

        case PassType::GBuffer:
            return std::make_unique<GBufferTechnique>(this, mCurrentRenderGraph);

        case PassType::SSAO:
            return std::make_unique<SSAOTechnique>(this, mCurrentRenderGraph);

        case PassType::GBufferPreDepth:
            return std::make_unique<GBufferPreDepthTechnique>(this, mCurrentRenderGraph);

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

        case PassType::ComputeSkinning:
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

        case PassType::RayTracedShadows:
            return std::make_unique<RayTracedShadowsTechnique>(this, mCurrentRenderGraph);

        case PassType::DownSampleColour:
            return std::make_unique<DownSampleColourTechnique>(this, mCurrentRenderGraph);

        case PassType::VoxelTerrain:
            return std::make_unique<VoxelTerrainTechnique>(this, mCurrentRenderGraph);

        case PassType::InstanceID:
            return std::make_unique<InstanceIDTechnique>(this, mCurrentRenderGraph);

        default:
        {
            BELL_TRAP;
            return nullptr;
        }
    }
}

std::pair<uint64_t, uint64_t> RenderEngine::addMeshToBuffer(const StaticMesh* mesh)
{
	const auto it = mVertexCache.find(mesh);
	if (it != mVertexCache.end())
		return it->second;

    const auto& vertexData = mesh->getVertexData();
    const auto& indexData = mesh->getIndexData();

	const auto vertexOffset = mVertexSize + mVertexBuilder.addData(vertexData);

    const auto indexOffset = mIndexSize + mIndexBuilder.addData(indexData);

	mVertexCache.insert(std::make_pair( mesh, std::pair<uint64_t, uint64_t>{vertexOffset, indexOffset}));

    return {vertexOffset, indexOffset};
}


std::pair<uint64_t, uint64_t> RenderEngine::addMeshToAnimationBuffer(const StaticMesh* mesh)
{
    const auto it = mTposeVertexCache.find(mesh);
    if(it != mTposeVertexCache.end())
        return it->second;

    const auto& vertexData = mesh->getVertexData();
    const auto vertexOffset = mAnimationVertexSize + mAnimationVertexBuilder.addData(vertexData);

    const std::vector<SubMesh>& subMeshes = mesh->getSubMeshes();
    uint32_t boneWeightsOffset = 0;
    for(const auto& subMesh : subMeshes)
    {
        const auto &boneIndicies = subMesh.mBoneWeightsIndicies;
        mBoneIndexBuilder.addData(boneIndicies);

        const auto &boneWeights = subMesh.mBoneWeights;
        boneWeightsOffset += mBoneWeightSize + mBoneWeightBuilder.addData(boneWeights);
    }

    mTposeVertexCache.insert(std::make_pair( mesh, std::pair<uint64_t, uint64_t>{vertexOffset, boneWeightsOffset}));

    return {vertexOffset, boneWeightsOffset};
}


uint64_t RenderEngine::getMeshBoundsIndex(const MeshInstance* inst)
{
    BELL_ASSERT(mMeshBoundsCache.find(inst) != mMeshBoundsCache.end(), "Unable to find instance bounds")

    return mMeshBoundsCache[inst];
}


void RenderEngine::execute(RenderGraph& graph)
{
    PROFILER_EVENT();

    // Upload vertex/index data if required.
    auto& vertexData = mVertexBuilder.finishRecording();
    auto& indexData = mIndexBuilder.finishRecording();
    if(!vertexData.empty() || !indexData.empty())
    {
        mVertexBuffer->resize(static_cast<uint32_t>(vertexData.size()), true);
        mIndexBuffer->resize(static_cast<uint32_t>(indexData.size()), true);

        mVertexBuffer->resize(vertexData.size(), false);
        mVertexBuffer->setContents(vertexData.data(), static_cast<uint32_t>(vertexData.size()), mVertexSize);
        mIndexBuffer->resize(indexData.size(), false);
        mIndexBuffer->setContents(indexData.data(), static_cast<uint32_t>(indexData.size()), mIndexSize);

        // upload mesh instance bounds info.
        static_assert(sizeof(AABB) == (sizeof(float4) * 2), "Sizes need to match");
        std::vector<AABB> bounds{};
        for(const auto& inst : mCurrentScene->getStaticMeshInstances())
        {
            mMeshBoundsCache.insert({&inst, bounds.size()});
            bounds.push_back(inst.getMesh()->getAABB() * inst.getTransMatrix());
        }
        for(const auto& inst : mCurrentScene->getDynamicMeshInstances())
        {
            mMeshBoundsCache.insert({&inst, bounds.size()});
            bounds.push_back(inst.getMesh()->getAABB() * inst.getTransMatrix());
        }

        if(!bounds.empty())
        {
            mMeshBoundsBuffer->resize(bounds.size() * sizeof(AABB), false);
            mMeshBoundsBuffer->setContents(bounds.data(), sizeof(AABB) * bounds.size());
        }

        mVertexSize += vertexData.size();
        mIndexSize += indexData.size();

        mVertexBuilder.reset();
        mIndexBuilder.reset();
    }

    auto& animationVerticies = mAnimationVertexBuilder.finishRecording();
    auto& boneWeights = mBoneWeightBuilder.finishRecording();
    auto& boneIndicies = mBoneIndexBuilder.finishRecording();
    if(!animationVerticies.empty() || !boneWeights.empty() || !boneIndicies.empty())
    {
        mTposeVertexBuffer->resize(static_cast<uint32_t>(mAnimationVertexSize + animationVerticies.size()), true);
        mBoneWeightsIndexBuffer->resize(static_cast<uint32_t>(mBoneIndexSize + boneIndicies.size()), true);
        mBonesWeightsBuffer->resize(static_cast<uint32_t>(mBoneWeightSize + boneWeights.size()), true);

        mTposeVertexBuffer->setContents(animationVerticies.data(), static_cast<uint32_t>(animationVerticies.size()), mAnimationVertexSize);
        mBoneWeightsIndexBuffer->setContents(boneIndicies.data(), static_cast<uint32_t>(boneIndicies.size()), mBoneIndexSize);
        mBonesWeightsBuffer->setContents(boneWeights.data(), static_cast<uint32_t>(boneWeights.size()), mBoneWeightSize);

        mAnimationVertexSize += animationVerticies.size();
        mBoneWeightSize += boneWeights.size();
        mBoneIndexSize += boneIndicies.size();

        mAnimationVertexBuilder.reset();
        mBoneWeightBuilder.reset();
        mBoneIndexBuilder.reset();
    }

    // Finalize graph internal state.
    if(mCompileGraph)
    {
        graph.compile(mRenderDevice);

        mCurrentRenderGraph.bindShaderResourceSet(kMaterials, mMaterials);
        mCurrentRenderGraph.bindImage(kLTCMat, mLTCMatView);
        mCurrentRenderGraph.bindImage(kLTCAmp, mLTCAmpView);
        mCurrentRenderGraph.bindImage(kBlueNoise, mBlueNoiseView);
        mCurrentRenderGraph.bindSampler(kDefaultSampler, mDefaultSampler);
        mCurrentRenderGraph.bindSampler(kPointSampler, mDefaultPointSampler);
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
    }

    for(const auto& tech : mTechniques)
    {
        tech->bindResources(mCurrentRenderGraph);
    }

    if(mCompileGraph)
    {
        for(auto& technique : mTechniques)
            technique->postGraphCompilation(graph, this);

        mCompileGraph = false;
    }

    updateGlobalBuffers();

    mMaterials->updateLastAccessed();
    mCurrentRenderGraph.bindBuffer(kCameraBuffer, *mDeviceCameraBuffer);
    mCurrentRenderGraph.bindBuffer(kShadowingLights, *mShadowCastingLight);

    if(!mActiveSkeletalAnimations.empty())
    {
        mCurrentRenderGraph.bindBuffer(kBonesBuffer, *mBoneBuffer);
    }

    if(mCurrentScene && mCurrentScene->getSkybox())
        (*mCurrentScene->getSkybox())->updateLastAccessed();

    std::vector<const MeshInstance*> meshes{};

    if(mCurrentScene)
    {
        meshes = mCurrentScene ? mCurrentScene->getVisibleMeshes(mCurrentScene->getCamera().getFrustum()) : std::vector<const MeshInstance*>{};
        PROFILER_EVENT("sort meshes");
        std::sort(meshes.begin(), meshes.end(), [camera = mCurrentScene->getCamera()](const MeshInstance* lhs, const MeshInstance* rhs)
        {
            const float3 centralLeft = (lhs->getMesh()->getAABB() *  lhs->getTransMatrix()).getCentralPoint();
            const float leftDistance = glm::length(centralLeft - camera.getPosition());

            const float3 centralright = (rhs->getMesh()->getAABB() * rhs->getTransMatrix()).getCentralPoint();
            const float rightDistance = glm::length(centralright - camera.getPosition());
            return leftDistance < rightDistance;
        });
    }

    //printf("Meshes %zu\n", meshes.size());

    auto barriers = graph.generateBarriers(mRenderDevice);
	
    uint32_t currentContext[static_cast<uint32_t>(QueueType::MaxQueues)] = { 0 };
    uint32_t submittedContexts[static_cast<uint32_t>(QueueType::MaxQueues)] = { 0 };
    if (mRecordTasksSync)
    {
        auto barrier = barriers.begin();
        const uint32_t taskCount = graph.taskCount();
        mAsyncTaskContextMappings.clear();
        mSyncTaskContextMappings.clear();

        for (uint32_t taskIndex = 0; taskIndex < taskCount; ++taskIndex)
        {
            RenderTask& task = graph.getTask(taskIndex);
            const uint32_t queueIndex = task.taskType() == TaskType::AsyncCompute ? 1 : 0;
            uint32_t currentContextIndex = currentContext[queueIndex];

            // Add this task to the correct context mapping entry.
            {
                std::vector<ContextMapping>& contexts = queueIndex == 1 ? mAsyncTaskContextMappings : mSyncTaskContextMappings;
                if(currentContextIndex + 1 > contexts.size())
                    contexts.resize(currentContextIndex + 1);

                ContextMapping& mapping = contexts[currentContextIndex];
                mapping.mTaskIndicies.push_back(taskIndex);
            }

            const QueueType queueType = static_cast<QueueType>(queueIndex);
            CommandContextBase* context = mRenderDevice->getCommandContext(currentContextIndex, queueType);
            Executor* exec = context->allocateExecutor(GPU_PROFILING);
            exec->recordBarriers(*barrier);
            context->setupState(graph, taskIndex, exec, mCurrentRegistredPasses);

            task.executeRecordCommandsCallback(graph, taskIndex, exec, this, meshes);

            context->freeExecutor(exec);
            ++barrier;

            if (context->getSubmitFlag()) // submit context
            {
                ++submittedContexts[queueIndex];
                ++currentContext[queueIndex];
                mRenderDevice->submitContext(context);
            }
        }

        // submit final async context if not already.
        if(submittedContexts[static_cast<uint32_t>(QueueType::Compute)] < currentContext[static_cast<uint32_t>(QueueType::Compute)])
        {
            const uint32_t asyncFinalContext = currentContext[static_cast<uint32_t>(QueueType::Compute)];
            CommandContextBase* asyncContext = mRenderDevice->getCommandContext(asyncFinalContext, QueueType::Compute);
            mRenderDevice->submitContext(asyncContext);
        }

        mRecordTasksSync = false;
    }
    else // Record tasks asynchronously.
    {
        // make sure we have enough contexst.
        mRenderDevice->getCommandContext(mSyncTaskContextMappings.size() - 1, QueueType::Graphics);
        if(!mAsyncTaskContextMappings.empty())
            mRenderDevice->getCommandContext(mAsyncTaskContextMappings.size() - 1, QueueType::Compute);

        auto recordToContext = [&](const ContextMapping& ctxMapping, const uint32_t contextIndex, const QueueType queue) -> CommandContextBase*
        {
            // Correct number of contexts will have been created when running tasks sync.
            CommandContextBase* context = mRenderDevice->getCommandContext(contextIndex, queue);

            for (uint32_t taskIndex : ctxMapping.mTaskIndicies)
            {
                RenderTask& task = graph.getTask(taskIndex);

                Executor* exec = context->allocateExecutor(GPU_PROFILING);
                exec->recordBarriers(barriers[taskIndex]);
                context->setupState(graph, taskIndex, exec, mCurrentRegistredPasses);

                task.executeRecordCommandsCallback(graph, taskIndex, exec, this, meshes);

                exec->resetRecordedCommandCount();

                context->freeExecutor(exec);
            }

            return context;
        };

        std::vector<std::future<CommandContextBase*>> resultHandles{};
        resultHandles.reserve(mAsyncTaskContextMappings.size());
        for (uint32_t i = 1; i < mSyncTaskContextMappings.size(); ++i)
        {
            resultHandles.push_back(mThreadPool.addTask(recordToContext, mSyncTaskContextMappings[i], i, QueueType::Graphics));
        }

        std::vector<std::future<CommandContextBase*>> asyncResultHandles{};
        asyncResultHandles.reserve(mAsyncTaskContextMappings.size());
        for(uint32_t i = 0; i < mAsyncTaskContextMappings.size(); ++i)
        {
            asyncResultHandles.push_back(mThreadPool.addTask(recordToContext, mAsyncTaskContextMappings[i], i, QueueType::Compute));
        }

        CommandContextBase* firstCtx = recordToContext(mSyncTaskContextMappings[0], 0, QueueType::Graphics);
        if(mSyncTaskContextMappings.size() > 1) // If this isn't the first and last context submit it now, if not we still need to record the frame buffer transition.
            mRenderDevice->submitContext(firstCtx);

        for(auto& ctx : asyncResultHandles)
        {
            CommandContextBase* context = ctx.get();
            mRenderDevice->submitContext(context);
        }

        for (uint32_t i = 0; i < resultHandles.size(); ++i)
        {
            std::future<CommandContextBase*>& result = resultHandles[i];

            CommandContextBase* context = result.get();

            if(&result != &resultHandles.back())
                mRenderDevice->submitContext(context);
        }

        currentContext[static_cast<uint32_t>(QueueType::Graphics)] = mSyncTaskContextMappings.size() - 1;
    }

    const uint32_t submissionContext = currentContext[static_cast<uint32_t>(QueueType::Graphics)];
    CommandContextBase* context = mRenderDevice->getCommandContext(submissionContext, QueueType::Graphics);
    Executor* exec = context->allocateExecutor();
	// Transition the swapchain image to a presentable format.
	BarrierRecorder frameBufferTransition{ mRenderDevice };
	auto& frameBufferView = getSwapChainImageView();
	frameBufferTransition->transitionLayout(frameBufferView, ImageLayout::Present, Hazard::ReadAfterWrite, SyncPoint::FragmentShaderOutput, SyncPoint::BottomOfPipe);

    exec->recordBarriers(frameBufferTransition);
    context->freeExecutor(exec);

    mRenderDevice->submitContext(context, true);
}


void RenderEngine::startAnimation(const InstanceID id, const std::string& name, const bool loop, const float speedModifer)
{
    BELL_ASSERT(mCurrentScene, "No scene set")
    const MeshInstance* inst = mCurrentScene->getMeshInstance(id);

    if(inst->getMesh()->isSkeletalAnimation(name))
        mActiveSkeletalAnimations.push_back({name, id, speedModifer, 0, 0.0, loop});

    if(inst->getMesh()->isBlendMeshAnimation(name))
        mActiveBlendShapeAnimations.push_back({name, id, speedModifer, 0.0, loop});
}


void RenderEngine::terimateAnimation(const InstanceID id, const std::string& name)
{
    if(!mActiveSkeletalAnimations.empty())
    {
        mActiveSkeletalAnimations.erase(std::remove_if(mActiveSkeletalAnimations.begin(), mActiveSkeletalAnimations.end(), [id, name](const SkeletalAnimationEntry& entry)
        {
            return entry.mName == name && entry.mMesh == id;
        }), mActiveSkeletalAnimations.end());
    }

    if(!mActiveBlendShapeAnimations.empty())
    {
        mActiveBlendShapeAnimations.erase(std::remove_if(mActiveBlendShapeAnimations.begin(), mActiveBlendShapeAnimations.end(), [id, name](const BlendShapeAnimationEntry& entry)
        {
            return entry.mName == name && entry.mMesh == id;
        }), mActiveBlendShapeAnimations.end());
    }
}


void RenderEngine::tickAnimations()
{
    PROFILER_EVENT();

    double elapsedTime = mFrameUpdateDelta.count();
    elapsedTime /= 1000000.0;
    uint64_t boneOffset = 0;
    std::vector<float4x4> boneMatracies{};

    for(auto& animEntry : mActiveSkeletalAnimations)
    {
        MeshInstance* instance = mCurrentScene->getMeshInstance(animEntry.mMesh);
        SkeletalAnimation& animation = instance->getMesh()->getSkeletalAnimation(animEntry.mName);
        double ticksPerSec = animation.getTicksPerSec();
        animEntry.mTick += elapsedTime * ticksPerSec * animEntry.mSpeedModifier;
        animEntry.mBoneOffset = boneOffset;

        if((animation.getTotalTicks() - animEntry.mTick) > 0.00001)
        {
            std::vector<float4x4> matracies  = animation.calculateBoneMatracies(*instance->getMesh(), animEntry.mTick);
            boneOffset += matracies.size();
            boneMatracies.insert(boneMatracies.end(), matracies.begin(), matracies.end());
        }
        else if(animEntry.mLoop)
            animEntry.mTick = 0.0;
    }

    for(auto& animEntry : mActiveBlendShapeAnimations)
    {
        MeshInstance* instance = mCurrentScene->getMeshInstance(animEntry.mMesh);
        BlendMeshAnimation& animation = instance->getMesh()->getBlendMeshAnimation(animEntry.mName);
        double ticksPerSec = animation.getTicksPerSec();
        animEntry.mTick += elapsedTime * ticksPerSec * animEntry.mSpeedModifier;

        if((animation.getTotalTicks() - animEntry.mTick) > 0.00001)
        {
        }
        else if(animEntry.mLoop)
            animEntry.mTick = 0.0;
    }

    if(!boneMatracies.empty())
        (*mBoneBuffer)->setContents(boneMatracies.data(), boneMatracies.size() * sizeof(float4x4));

    mActiveSkeletalAnimations.erase(std::remove_if(mActiveSkeletalAnimations.begin(), mActiveSkeletalAnimations.end(), [this](const SkeletalAnimationEntry& entry)
    {
        SkeletalAnimation& animation = mCurrentScene->getSkeletalAnimation(entry.mMesh, entry.mName);
        return (entry.mTick - animation.getTotalTicks()) > 0.00001;
    }), mActiveSkeletalAnimations.end());

    mActiveBlendShapeAnimations.erase(std::remove_if(mActiveBlendShapeAnimations.begin(), mActiveBlendShapeAnimations.end(), [this](const BlendShapeAnimationEntry& entry)
    {
        BlendMeshAnimation& animation = mCurrentScene->getBlendMeshAnimation(entry.mMesh, entry.mName);
        return (entry.mTick - animation.getTotalTicks()) > 0.00001;
    }), mActiveBlendShapeAnimations.end());
}


void RenderEngine::recordScene()
{   
    PROFILER_EVENT();

    // Add new techniques
    if (mPassesRegisteredThisFrame > 0)
    {
        mCurrentRegistredPasses |= mPassesRegisteredThisFrame;

        while (mPassesRegisteredThisFrame > 0)
        {
            static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "builtin requires sizes be the same ");
#ifdef _MSC_VER
            unsigned long passTypeIndex;
            _BitScanForward64(&passTypeIndex, mPassesRegisteredThisFrame);
#else
            const uint64_t passTypeIndex = __builtin_ctzll(mPassesRegisteredThisFrame);
#endif
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


void RenderEngine::render()
{
    PROFILER_EVENT();

	execute(mCurrentRenderGraph);
}


void RenderEngine::updateGlobalBuffers()
{
    PROFILER_EVENT();

    if(!mInitialisedTLCTextures)
    {
        // Load textures for LTC.
        {
            TextureUtil::TextureInfo matInfo = TextureUtil::load128BitTexture("./Assets/ltc_mat.hdr", 4);
            mLTCMat->setContents(matInfo.mData.data(), matInfo.width, matInfo.height, 1);
        }

        {
            TextureUtil::TextureInfo ampInfo = TextureUtil::load128BitTexture("./Assets/ltc_amp.hdr", 2);
            mLTCAmp->setContents(ampInfo.mData.data(), ampInfo.width, ampInfo.height, 1);
        }

        {
            TextureUtil::TextureInfo noiseInfo = TextureUtil::load32BitTexture("./Assets/BlueNoise.png", 1);
            mBlueNoise->setContents(noiseInfo.mData.data(), 470, 470, 1);
        }

        {
            TextureUtil::TextureInfo defaultInfo = TextureUtil::load32BitTexture("./Assets/DefaultDiffuse.png", 4);
            mDefaultDiffuseTexture->setContents(defaultInfo.mData.data(), 4, 4, 1);
        }

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
            mCameraBuffer.mPreviousFrameInvertedViewProjMatrix = mCameraBuffer.mInvertedViewProjMatrix;
            mCameraBuffer.mPreviousViewMatrix = mCameraBuffer.mViewMatrix;
            // need to add jitter for TAA
            if(isPassRegistered(PassType::TAA))
            {
                const uint32_t index = mRenderDevice->getCurrentSubmissionIndex() % 16;
                const float2& jitter = mTAAJitter[index];

                perspective = glm::translate(float3(jitter / mCameraBuffer.mFrameBufferSize, 0.0f)) * currentCamera.getProjectionMatrix();
                mCameraBuffer.mPreviousJitter = mCameraBuffer.mJitter;
                mCameraBuffer.mJitter = jitter / mCameraBuffer.mFrameBufferSize;
            }
            else
            {
                perspective = currentCamera.getProjectionMatrix();
                mCameraBuffer.mPreviousJitter = mCameraBuffer.mJitter;
                mCameraBuffer.mJitter = float2(0.0f, 0.0f);
            }
            mCameraBuffer.mViewProjMatrix = perspective * view;
            mCameraBuffer.mInvertedViewProjMatrix = glm::inverse(mCameraBuffer.mViewProjMatrix);
            mCameraBuffer.mInvertedPerspective = glm::inverse(perspective);
            mCameraBuffer.mViewMatrix = view;
            mCameraBuffer.mInvertedView = glm::inverse(view);
            mCameraBuffer.mPerspectiveMatrix = perspective;
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
            mCurrentScene->updateShadowingLight();
            const Scene::ShadowingLight& shadowingLight = mCurrentScene->getShadowingLight();

            MapInfo mapInfo{};
            mapInfo.mSize = sizeof(Scene::ShadowingLight);
            mapInfo.mOffset = 0;

            void* dst = mShadowCastingLight.get()->map(mapInfo);

                std::memcpy(dst, &shadowingLight, sizeof(Scene::ShadowingLight));

            mShadowCastingLight.get()->unmap();
        }

        if(!mActiveSkeletalAnimations.empty())
        {
            tickAnimations();
        }
    }
}


void RenderEngine::registerPass(const PassType pass)
{
	if((static_cast<uint64_t>(pass) & mCurrentRegistredPasses) == 0)
	{
        mShaderPrefix.push_back(ShaderDefine(std::string(passToString(pass)), 1));

        mPassesRegisteredThisFrame |= static_cast<uint64_t>(pass);
	}
}


bool RenderEngine::isPassRegistered(const PassType pass) const
{
	return (static_cast<uint64_t>(pass) & mCurrentRegistredPasses) > 0;
}


void RenderEngine::rayTraceScene()
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


CPUImage RenderEngine::renderDiffuseCubeMap(const RayTracingScene& scene, const float3 &position, const uint32_t x, const uint32_t y)
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


std::vector<RenderEngine::SphericalHarmonic> RenderEngine::generateIrradianceProbes(const std::vector<IrradianceProbeVolume>& volumes)
{
    std::vector<float3> positions{};
    for(const auto& volume : volumes)
    {
        const auto probePositions = volume.getProbePositions();
        positions.insert(positions.end(), probePositions.begin(), probePositions.end());
    }

    return generateIrradianceProbes(positions);
}


std::vector<RenderEngine::SphericalHarmonic> RenderEngine::generateIrradianceProbes(const std::vector<float3>& positions)
{
    std::vector<RenderEngine::SphericalHarmonic> harmonics{};

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
                                                          sizeof(RenderEngine::SphericalHarmonic) * harmonics.size(), sizeof(RenderEngine::SphericalHarmonic) * harmonics.size(),
                                                          "Irradiance harmonics");
        mIrradianceProbeBufferView = std::make_unique<BufferView>(*mIrradianceProbeBuffer);

        (*mIrradianceProbeBuffer)->setContents(harmonics.data(), sizeof(RenderEngine::SphericalHarmonic) * harmonics.size());
    }

    return harmonics;
}


RenderEngine::SphericalHarmonic RenderEngine::generateSphericalHarmonic(const float3& position, const CPUImage& cubemap)
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


std::vector<RenderEngine::KdNode> RenderEngine::generateProbeKdTree(std::vector<SphericalHarmonic> &harmonics)
{
    std::vector<RenderEngine::KdNode> treeNodes{};

    std::function<int32_t(const uint32_t, std::vector<uint32_t>&, std::vector<RenderEngine::KdNode>&)> buildKdTreeRecurse =
            [&](const uint32_t depth, std::vector<uint32_t>& nodeHarmonicsindicies, std::vector<RenderEngine::KdNode>& results) -> int32_t
    {
        if(nodeHarmonicsindicies.empty())
            return -1;

        const uint32_t axis = depth % 3u;
        const uint32_t halfIndex = nodeHarmonicsindicies.size() / 2u;
        std::nth_element(nodeHarmonicsindicies.begin(), nodeHarmonicsindicies.begin() + halfIndex, nodeHarmonicsindicies.end(), [&](const uint32_t lhs, const uint32_t rhs)
        {
            return harmonics[lhs].mPosition[axis] < harmonics[rhs].mPosition[axis];
        });

        std::vector<uint32_t> leftChildren{nodeHarmonicsindicies.begin(), nodeHarmonicsindicies.begin() + halfIndex};
        std::vector<uint32_t> rightChildren{nodeHarmonicsindicies.begin() + halfIndex + 1u, nodeHarmonicsindicies.end()};

        const uint32_t newNodeIndex = results.size();
        uint32_t& pivot = nodeHarmonicsindicies[halfIndex];
        results.push_back(KdNode{harmonics[pivot].mPosition, pivot, -1, -1, {}});
        KdNode& newNode = results[newNodeIndex];

        uint32_t leftChildrenIndex = buildKdTreeRecurse(depth + 1u, leftChildren, results);
        newNode.mLeftIndex = leftChildrenIndex;
        uint32_t rightChildrenIndex = buildKdTreeRecurse(depth + 1u, rightChildren, results);
        newNode.mRightIndex = rightChildrenIndex;

        return newNodeIndex;

    };

    std::vector<uint32_t> harmonicIndicies{};
    harmonicIndicies.resize(harmonics.size());
    std::iota(harmonicIndicies.begin(), harmonicIndicies.end(), 0);
    buildKdTreeRecurse(0u, harmonicIndicies, treeNodes);

    // rebaking to need to flush and reset.
    if(mIrradianceProbeBuffer)
    {
        mRenderDevice->flushWait();

        mLightProbeResourceSet.reset(mRenderDevice, 2);
    }

    mIrradianceKdTreeBuffer = std::make_unique<Buffer>(mRenderDevice, BufferUsage::DataBuffer,
                                                                 sizeof(KdNode) * treeNodes.size(), sizeof(KdNode) * treeNodes.size(), "Irradiance probe KdTree");

    mIrradianceKdTreeBufferView = std::make_unique<BufferView>(*mIrradianceKdTreeBuffer);

    (*mIrradianceKdTreeBuffer)->setContents(treeNodes.data(), treeNodes.size() * sizeof(KdNode));

    // Set up light probe SRS.
    mLightProbeResourceSet->addDataBufferRO(*mIrradianceProbeBufferView);
    mLightProbeResourceSet->addDataBufferRO(*mIrradianceKdTreeBufferView);
    mLightProbeResourceSet->finalise();

    mIrradianceProbesHarmonics = std::move(harmonics);

    return treeNodes;
}


void RenderEngine::loadIrradianceProbes(const std::string& probesPath, const std::string& lookupPath)
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
                                                      sizeof(RenderEngine::SphericalHarmonic) * mIrradianceProbesHarmonics.size(), sizeof(RenderEngine::SphericalHarmonic) * mIrradianceProbesHarmonics.size(),
                                                      "Irradiance harmonics");
    mIrradianceProbeBufferView = std::make_unique<BufferView>(*mIrradianceProbeBuffer);

    (*mIrradianceProbeBuffer)->setContents(mIrradianceProbesHarmonics.data(), sizeof(RenderEngine::SphericalHarmonic) * mIrradianceProbesHarmonics.size());

    // load lookup texture.
    FILE* lookupFile = fopen(lookupPath.c_str(), "rb");
    BELL_ASSERT(lookupFile, "Unable to find irradiance probe file")
    fseek(lookupFile, 0L, SEEK_END);
    const uint32_t lookupSize = ftell(lookupFile);
    fseek(lookupFile, 0L, SEEK_SET);

    BELL_ASSERT((lookupSize % sizeof(KdNode)) == 0, "Invalid irradiance probe lookup tree cache size")
    std::vector<KdNode> treeData{};
    treeData.resize(lookupSize / sizeof(KdNode));
    fread(treeData.data(), sizeof(KdNode), treeData.size(), lookupFile);
    fclose(lookupFile);

    if(mIrradianceKdTreeBuffer)
    {
        mRenderDevice->flushWait();

        mLightProbeResourceSet.reset(mRenderDevice, 2);
    }

    mIrradianceKdTreeBuffer = std::make_unique<Buffer>(mRenderDevice, BufferUsage::DataBuffer,
                                                                 treeData.size() * sizeof(KdNode), treeData.size() * sizeof(KdNode), "Voronoi probe lookup");
    mIrradianceKdTreeBufferView = std::make_unique<BufferView>(*mIrradianceKdTreeBuffer);

    (*mIrradianceKdTreeBuffer)->setContents(treeData.data(), treeData.size() * sizeof(KdNode));

    // Set up light probe SRS.
    mLightProbeResourceSet->addDataBufferRO(*mIrradianceProbeBufferView);
    mLightProbeResourceSet->addDataBufferRO(*mIrradianceKdTreeBufferView);
    mLightProbeResourceSet->finalise();
}


Technique* RenderEngine::getRegisteredTechnique(const PassType pass)
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



std::vector<float3> RenderEngine::IrradianceProbeVolume::getProbePositions() const
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
