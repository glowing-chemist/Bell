#include "Core/ConversionUtils.hpp"
#include "Core/BellLogging.hpp"

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


Engine::Engine(GLFWwindow* windowPtr) :
    mRenderInstance(windowPtr),
    mRenderDevice(mRenderInstance.createRenderDevice(DeviceFeaturesFlags::Compute | DeviceFeaturesFlags::Discrete | DeviceFeaturesFlags::Subgroup)),
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
    mCameraBuffer{},
	mDeviceCameraBuffer{getDevice(), BufferUsage::Uniform, sizeof(CameraBuffer), sizeof(CameraBuffer), "Camera Buffer"},
	mSSAOBUffer{},
	mDeviceSSAOBuffer{getDevice(), BufferUsage::Uniform, sizeof(SSAOBuffer), sizeof(SSAOBuffer), "SSAO Buffer"},
	mGeneratedSSAOBuffer{false},
    mLightBuffer(getDevice(), BufferUsage::DataBuffer | BufferUsage::TransferDest, (sizeof(Scene::Light) * 1000) + sizeof(uint32_t), (sizeof(Scene::Light) * 1000) + sizeof(uint32_t), "LightBuffer"),
    mLightBufferView(mLightBuffer),
    mLightsSRS(getDevice()),
    mWindow(windowPtr)
{
    for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
    {
        mLightsSRS.get(i).addDataBufferRW(mLightBufferView.get(i));
        mLightsSRS.get(i).finalise();
    }

}


void Engine::loadScene(const std::string& path)
{
    mLoadingScene = Scene(path);
	mLoadingScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals | VertexAttributes::Material, this);

	// Set up the SRS for the materials.
	const auto& materials = mCurrentScene.getMaterials();
	mMaterials.addSampledImageArray(materials);
	mMaterials.finalise();
}

void Engine::transitionScene()
{
    mCurrentScene = std::move(mLoadingScene);
}


void Engine::setScene(const std::string& path)
{
    mCurrentScene = Scene(path);
    mCurrentScene.loadFromFile(VertexAttributes::Position4 | VertexAttributes::TextureCoordinates | VertexAttributes::Normals | VertexAttributes::Material, this);

	// Set up the SRS for the materials.
	const auto& materials = mCurrentScene.getMaterials();
	mMaterials.addSampledImageArray(materials);
	mMaterials.finalise();
}


void Engine::setScene(Scene& scene)
{
    mCurrentScene = std::move(scene);

	// Set up the SRS for the materials.
	const auto& materials = mCurrentScene.getMaterials();
	mMaterials.addSampledImageArray(materials);
	mMaterials.finalise();
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
    return Image{&mRenderDevice, format, usage, x, y, z, mips, levels, samples, name};
}

Buffer Engine::createBuffer(const uint32_t size,
					const uint32_t stride,
					BufferUsage usage,
					const std::string& name)
{
	return Buffer{&mRenderDevice, usage, size, stride, name};
}


Shader Engine::getShader(const std::string& path)
{
	if(mShaderCache.find(path) != mShaderCache.end())
		return (*mShaderCache.find(path)).second;

	Shader newShader{&mRenderDevice, path};

	const bool compiled = newShader.compile();

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


void Engine::recordScene()
{
	const std::vector<const Scene::MeshInstance*> meshes = mCurrentScene.getViewableMeshes();

	BELL_ASSERT(!mTechniques.empty(), "Need at least one technique registered with the engine")

	for(auto& tech : mTechniques)
	{
		tech->render(mCurrentRenderGraph, this, meshes);
	}

	for(auto& tech : mTechniques)
	{
		tech->bindResources(mCurrentRenderGraph);
	}

    updateGlobalBuffers();

	mCurrentRenderGraph.bindShaderResourceSet(kMaterials, mMaterials);
    mCurrentRenderGraph.bindBuffer(kCameraBuffer, *mDeviceCameraBuffer);
    mCurrentRenderGraph.bindShaderResourceSet(kLightBuffer, *mLightsSRS);
    mCurrentRenderGraph.bindBuffer(kSSAOBuffer, mDeviceSSAOBuffer);
    mCurrentRenderGraph.bindSampler(kDefaultSampler, mDefaultSampler);

	if(mCurrentScene.getSkybox())
		mCurrentRenderGraph.bindImage(kSkyBox, *mCurrentScene.getSkybox());
}


void Engine::render()
{
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

        mCurrentRenderGraph.bindVertexBuffer(*mVertexBuffer);
        mCurrentRenderGraph.bindIndexBuffer(*mIndexBuffer);
    }

	mCurrentRenderGraph.compileDependancies();

    mRenderDevice.execute(mCurrentRenderGraph);
}


void Engine::submitCommandRecorder(CommandContext &ccx)
{
    RenderGraph& renderGraph = ccx.finialise();

    auto& vertexData = mVertexBuilder.finishRecording();

    auto& indexData = mIndexBuilder.finishRecording();

    mVertexBuffer->resize(static_cast<uint32_t>(vertexData.size()), false);
    mIndexBuffer->resize(static_cast<uint32_t>(indexData.size()), false);

    mVertexBuffer->setContents(vertexData.data(), static_cast<uint32_t>(vertexData.size()));
    mIndexBuffer->setContents(indexData.data(), static_cast<uint32_t>(indexData.size()));

    mVertexBuilder.reset();
    mIndexBuilder.reset();

    renderGraph.bindVertexBuffer(*mVertexBuffer);
    renderGraph.bindIndexBuffer(*mIndexBuffer);

    mRenderDevice.execute(renderGraph);
}


void Engine::updateGlobalBuffers()
{
	auto& currentCamera = getCurrentSceneCamera();

	mCameraBuffer.mViewMatrix = currentCamera.getViewMatrix();
	mCameraBuffer.mPerspectiveMatrix = currentCamera.getPerspectiveMatrix();
	mCameraBuffer.mInvertedCameraMatrix = glm::inverse(mCameraBuffer.mPerspectiveMatrix * mCameraBuffer.mViewMatrix);
	mCameraBuffer.mInvertedPerspective = glm::inverse(mCameraBuffer.mPerspectiveMatrix);
	mCameraBuffer.mFrameBufferSize = glm::vec2{getSwapChainImage().getExtent(0, 0).width, getSwapChainImage().getExtent(0, 0).height};
	mCameraBuffer.mPosition = currentCamera.getPosition();
	mCameraBuffer.mNeaPlane = currentCamera.getNearPlane();
	mCameraBuffer.mFarPlane = currentCamera.getFarPlane();
    mCameraBuffer.mFOV = currentCamera.getFOV();

	MapInfo mapInfo{};
	mapInfo.mSize = sizeof(CameraBuffer);
	mapInfo.mOffset = 0;

    void* cameraBufferPtr = mDeviceCameraBuffer->map(mapInfo);

		std::memcpy(cameraBufferPtr, &mCameraBuffer, sizeof(CameraBuffer));

    mDeviceCameraBuffer->unmap();

	// The SSAO buffer only needs tp be updated once.
	// Or if the number of samples needs to be changed.
	if(!mGeneratedSSAOBuffer)
	{
		std::array<float3, 16> offsets;

		for(uint32_t i = 0; i < 16; ++i)
		{
			const float r1 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;
			const float r2 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)); // generate vectors on a hemisphere.
			const float r3 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;

			const float3 vec = glm::normalize(float3{r1, r2, r3});

			offsets[i] = float4{vec, 0.0f};
		}

		mGeneratedSSAOBuffer = true;

        std::memcpy(&mSSAOBUffer.mOffsets[0], offsets.data(), offsets.size() * sizeof(float3));
		mSSAOBUffer.mScale = 0.001f;
		mSSAOBUffer.mOffsetsCount = offsets.size();

		mapInfo.mSize = sizeof(SSAOBuffer);

		void* SSAOBufferPtr = mDeviceSSAOBuffer.map(mapInfo);

			std::memcpy(SSAOBufferPtr, &mSSAOBUffer, sizeof(SSAOBuffer));

		mDeviceSSAOBuffer.unmap();
	}

    mLightBuffer->setContents(static_cast<int>(mCurrentScene.getLights().size()), sizeof(uint32_t));

    if(!mCurrentScene.getLights().empty())
        mLightBuffer->setContents(mCurrentScene.getLights().data(), static_cast<uint32_t>(mCurrentScene.getLights().size() * sizeof(Scene::Light)), sizeof(uint32_t));
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
