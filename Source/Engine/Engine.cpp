#include "Core/ConversionUtils.hpp"
#include "Core/BellLogging.hpp"

#ifdef VULKAN
#include "Core/Vulkan/VulkanRenderInstance.hpp" 
#endif

#ifdef OPENGL
#include "Core/OpenGL/OpenGLRenderInstance.hpp"
#endif

#include "Engine/Engine.hpp"
#include "Engine/PreDepthTechnique.hpp"
#include "Engine/GBufferTechnique.hpp"
#include "Engine/GBufferMaterialTechnique.hpp"
#include "Engine/SSAOTechnique.hpp"
#include "Engine/BlurXTechnique.hpp"
#include "Engine/BlurYTechnique.hpp"
#include "Engine/BlinnPhongTechnique.hpp"
#include "Engine/OverlayTechnique.hpp"
#include "Engine/AnalyticalImageBasedLightingTechnique.hpp"
#include "Engine/ImageBasedLightingTechnique.hpp"
#include "Engine/DFGGenerationTechnique.hpp"
#include "Engine/ConvolveSkyboxTechnique.hpp"
#include "Engine/SkyboxTechnique.hpp"
#include "Engine/CompositeTechnique.hpp"
#include "Engine/ForwardIBLTechnique.hpp"
#include "Engine/LightFroxelationTechnique.hpp"
#include "Engine/DeferredAnalyticalLightingTechnique.hpp"
#include "Engine/ShadowMappingTechnique.hpp"


Engine::Engine(GLFWwindow* windowPtr) :
#ifdef VULKAN
    mRenderInstance( new VulkanRenderInstance(windowPtr)),
#endif
#ifdef OPENGL
    mRenderInstance( new OpenGLRenderInstance(windowPtr)),
#endif
    mRenderDevice(mRenderInstance->createRenderDevice(DeviceFeaturesFlags::Compute | DeviceFeaturesFlags::Subgroup)),
    mCurrentScene("Initial current scene"),
    mLoadingScene("Initial loading scene"),
    mVertexBuilder(),
    mIndexBuilder(),
    mMaterials{getDevice()},
    mCurrentRenderGraph(),
	mTechniques{},
	mCurrentPasstypes{0},
	mCommandContext(),
    mVertexBuffer{getDevice(), BufferUsage::Vertex | BufferUsage::TransferDest, 1000000, 1000000, "Vertex Buffer"},
    mIndexBuffer{getDevice(), BufferUsage::Index | BufferUsage::TransferDest, 1000000, 1000000, "Index Buffer"},
    mDefaultSampler(SamplerType::Linear),
    mShowDebugTexture(false),
    mDebugTextureName(""),
    mCameraBuffer{},
	mDeviceCameraBuffer{getDevice(), BufferUsage::Uniform, sizeof(CameraBuffer), sizeof(CameraBuffer), "Camera Buffer"},
    mShadowCastingLight(getDevice(), BufferUsage::Uniform, sizeof(Scene::ShadowingLight), sizeof(Scene::ShadowingLight), "ShadowingLight"),
    mLightBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, (sizeof(Scene::Light) * 1000) + sizeof(uint32_t), (sizeof(Scene::Light) * 1000) + sizeof(uint32_t), "LightBuffer"),
    mLightBufferView(mLightBuffer),
    mLightsSRS(getDevice()),
    mWindow(windowPtr)
{
    for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
    {
        mLightsSRS.get(i)->addDataBufferRW(mLightBufferView.get(i));
        mLightsSRS.get(i)->finalise();
    }

}


void Engine::loadScene(const std::string& path)
{
    mLoadingScene = Scene(path);
	mLoadingScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals | VertexAttributes::Material, this);

    mVertexCache.clear();
    mLoadingScene.finalise(this);

    setScene(mCurrentScene);
}

void Engine::transitionScene()
{
    mVertexCache.clear();
    mCurrentScene = std::move(mLoadingScene);
}


void Engine::setScene(const std::string& path)
{
    mCurrentScene = Scene(path);
    mCurrentScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals | VertexAttributes::Material, this);

    mVertexCache.clear();
    mCurrentScene.finalise(this);

    setScene(mCurrentScene);
}


void Engine::setScene(Scene& scene)
{
    mCurrentScene = std::move(scene);

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
    }

	// Set up the SRS for the materials.
	const auto& materials = mCurrentScene.getMaterials();
	mMaterials->addSampledImageArray(materials);
	mMaterials->finalise();
}


Camera& Engine::getCurrentSceneCamera()
{
    return mCurrentScene.getCamera();
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
	if(mShaderCache.find(path) != mShaderCache.end())
		return (*mShaderCache.find(path)).second;

	Shader newShader{mRenderDevice, path};

	const bool compiled = newShader->compile();

	BELL_ASSERT(compiled, "Shader failed to compile")

	mShaderCache.insert({path, newShader});

	return newShader;
}


std::unique_ptr<Technique> Engine::getSingleTechnique(const PassType passType)
{
    switch(passType)
    {
        case PassType::DepthPre:
            return std::make_unique<PreDepthTechnique>(this);

        case PassType::GBuffer:
            return std::make_unique<GBufferTechnique>(this);

        case PassType::GBufferMaterial:
            return std::make_unique<GBufferMaterialTechnique>(this);

        case PassType::SSAO:
            return std::make_unique<SSAOTechnique>(this);

		case PassType::SSAOImproved:
			return std::make_unique<SSAOImprovedTechnique>(this);

        case PassType::GBufferPreDepth:
            return std::make_unique<GBufferPreDepthTechnique>(this);

        case PassType::GBUfferMaterialPreDepth:
            return std::make_unique<GBufferMaterialPreDepthTechnique>(this);

        case PassType::DeferredTextureBlinnPhongLighting:
            return std::make_unique<BlinnPhongDeferredTexturesTechnique>(this);

		case PassType::Overlay:
			return std::make_unique<OverlayTechnique>(this);

		case PassType::DeferredTextureAnalyticalPBRIBL:
			return std::make_unique<AnalyticalImageBasedLightingTechnique>(this);

		case PassType::DeferredTexturePBRIBL:
			return std::make_unique<ImageBasedLightingDeferredTexturingTechnique>(this);

        case PassType::DeferredPBRIBL:
            return std::make_unique<DeferredImageBasedLightingTechnique>(this);

		case PassType::ConvolveSkybox:
			return std::make_unique<ConvolveSkyBoxTechnique>(this);

		case PassType::Skybox:
			return std::make_unique<SkyboxTechnique>(this);

		case PassType::DFGGeneration:
			return std::make_unique<DFGGenerationTechnique>(this);

		case PassType::Composite:
			return std::make_unique<CompositeTechnique>(this);

		case PassType::ForwardIBL:
			return std::make_unique<ForwardIBLTechnique>(this);

		case PassType::LightFroxelation:
			return std::make_unique<LightFroxelationTechnique>(this);

		case PassType::DeferredAnalyticalLighting:
			return std::make_unique<DeferredAnalyticalLightingTechnique>(this);

        case PassType::Shadow:
            return std::make_unique<ShadowMappingTechnique>(this);

        default:
        {
            BELL_TRAP;
            return nullptr;
        }
    }
}


bool Engine::isGraphicsTask(const PassType passType) const
{
    switch(passType)
    {
        case PassType::DepthPre:
            return true;

        case PassType::GBuffer:
            return true;

        case PassType::GBufferMaterial:
            return true;

        case PassType::SSAO:
            return true;

        case PassType::GBufferPreDepth:
            return true;

        case PassType::GBUfferMaterialPreDepth:
            return true;

        case PassType::DeferredTextureBlinnPhongLighting:
            return true;

        default:
        {
            return false;
        }
    }
}


bool Engine::isComputeTask(const PassType pass) const
{
	switch(pass)
	{
		case PassType::DFGGeneration:
			return true;

		case PassType::ConvolveSkybox:
			return true;

		case PassType::LightFroxelation:
			return true;

		default:

			return false;
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


void Engine::execute(RenderGraph& graph)
{
	// Finalize graph internal state.
	graph.compileDependancies();
	graph.generateTransientImages(mRenderDevice);
	graph.reorderTasks();
	graph.mergeTasks();

	mRenderDevice->generateFrameResources(graph);

    auto barriers = graph.generateBarriers(mRenderDevice);

	// process scene.
    auto barrier = barriers.begin();
    uint32_t taskIndex = 0;
    for (auto task = graph.taskBegin(); task != graph.taskEnd(); ++task, ++barrier, ++taskIndex)
	{

        mRenderDevice->execute(*barrier);

		mRenderDevice->startPass(*task);

		Executor* exec = mRenderDevice->getPassExecutor();
		BELL_ASSERT(exec, "Failed to create executor")

        (*task).recordCommands(*exec, graph, taskIndex);

        mRenderDevice->freePassExecutor(exec);

		mRenderDevice->endPass();
	}

	// Transition the swapchain image to a presentable format.
	BarrierRecorder frameBufferTransition{ mRenderDevice };
	auto& frameBufferView = getSwapChainImageView();
	frameBufferTransition->transitionLayout(frameBufferView, ImageLayout::Present, Hazard::ReadAfterWrite, SyncPoint::FragmentShaderOutput, SyncPoint::BottomOfPipe);

	mRenderDevice->execute(frameBufferTransition);

	mRenderDevice->submitFrame();
}


void Engine::recordScene()
{
    const std::vector<const Scene::MeshInstance*> meshes = mCurrentScene.getViewableMeshes(mCurrentScene.getCamera().getFrustum());

	BELL_ASSERT(!mTechniques.empty(), "Need at least one technique registered with the engine")

	for(auto& tech : mTechniques)
	{
		tech->render(mCurrentRenderGraph, this, meshes);
	}

	for(const auto& tech : mTechniques)
	{
		tech->bindResources(mCurrentRenderGraph);
	}

    updateGlobalBuffers();

	mCurrentRenderGraph.bindShaderResourceSet(kMaterials, mMaterials);
    mCurrentRenderGraph.bindBuffer(kCameraBuffer, *mDeviceCameraBuffer);
	mCurrentRenderGraph.bindBuffer(kShadowingLights, *mShadowCastingLight);
    mCurrentRenderGraph.bindShaderResourceSet(kLightBuffer, *mLightsSRS);
    mCurrentRenderGraph.bindSampler(kDefaultSampler, mDefaultSampler);
    mCurrentRenderGraph.bindVertexBuffer(kSceneVertexBuffer, mVertexBuffer);
    mCurrentRenderGraph.bindIndexBuffer(kSceneIndexBuffer, mIndexBuffer);

	if(mCurrentScene.getSkybox())
		mCurrentRenderGraph.bindImage(kSkyBox, *mCurrentScene.getSkybox());
}


void Engine::render()
{
	execute(mCurrentRenderGraph);
}


void Engine::submitCommandRecorder(CommandContext &ccx)
{
    RenderGraph& renderGraph = ccx.finialise();

    mVertexBuilder.reset();
    mIndexBuilder.reset();

    renderGraph.bindBuffer(kSceneVertexBuffer, mVertexBuffer);
    renderGraph.bindBuffer(kSceneIndexBuffer, mIndexBuffer);

    execute(renderGraph);
}


void Engine::updateGlobalBuffers()
{
	{
		auto& currentCamera = getCurrentSceneCamera();

		mCameraBuffer.mViewMatrix = currentCamera.getViewMatrix();
		mCameraBuffer.mPerspectiveMatrix = currentCamera.getPerspectiveMatrix();
		mCameraBuffer.mInvertedCameraMatrix = glm::inverse(mCameraBuffer.mPerspectiveMatrix * mCameraBuffer.mViewMatrix);
		mCameraBuffer.mInvertedPerspective = glm::inverse(mCameraBuffer.mPerspectiveMatrix);
		mCameraBuffer.mFrameBufferSize = glm::vec2{ getSwapChainImage()->getExtent(0, 0).width, getSwapChainImage()->getExtent(0, 0).height };
		mCameraBuffer.mPosition = currentCamera.getPosition();
		mCameraBuffer.mNeaPlane = currentCamera.getNearPlane();
		mCameraBuffer.mFarPlane = currentCamera.getFarPlane();
		mCameraBuffer.mFOV = currentCamera.getFOV();

		MapInfo mapInfo{};
		mapInfo.mSize = sizeof(CameraBuffer);
		mapInfo.mOffset = 0;

		void* cameraBufferPtr = mDeviceCameraBuffer.get()->map(mapInfo);

		std::memcpy(cameraBufferPtr, &mCameraBuffer, sizeof(CameraBuffer));

		mDeviceCameraBuffer.get()->unmap();
	}

    {
        mLightBuffer.get()->setContents(static_cast<int>(mCurrentScene.getLights().size()), sizeof(uint32_t));

        if(!mCurrentScene.getLights().empty())
            mLightBuffer.get()->setContents(mCurrentScene.getLights().data(), static_cast<uint32_t>(mCurrentScene.getLights().size() * sizeof(Scene::Light)), 
                sizeof(float4));
    }

    {
        const Scene::ShadowingLight& shadowingLight = mCurrentScene.getShadowingLight();

		MapInfo mapInfo{};
		mapInfo.mSize = sizeof(Scene::ShadowingLight);
		mapInfo.mOffset = 0;

        void* dst = mShadowCastingLight.get()->map(mapInfo);

            std::memcpy(dst, &shadowingLight, sizeof(Scene::ShadowingLight));

        mShadowCastingLight.get()->unmap();
    }
}


void Engine::registerPass(const PassType pass)
{
	if((static_cast<uint64_t>(pass) & mCurrentPasstypes) == 0)
	{
		mTechniques.push_back(getSingleTechnique(pass));

		mCurrentPasstypes |= static_cast<uint64_t>(pass);
	}
}


bool Engine::isPassRegistered(const PassType pass) const
{
	return (static_cast<uint64_t>(pass) & mCurrentPasstypes) > 0;
}
