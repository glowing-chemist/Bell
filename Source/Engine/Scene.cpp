#include "Engine/Scene.h"
#include "Engine/Engine.hpp"
#include "Core/BellLogging.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "stb_image.h"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>

namespace
{
	struct TextureInfo
	{
		std::vector<unsigned char> mData;
		int width;
		int height;
	};

	TextureInfo loadTexture(const char* filePath, int chanels)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, chanels);

        BELL_LOG_ARGS("loading texture file: %s", filePath)
        //BELL_ASSERT(texChannels == chanels, "Texture file has a different ammount of channels than requested")

		std::vector<unsigned char> imageData{};
        imageData.resize(texWidth * texHeight * 4);

        std::memcpy(imageData.data(), pixels, texWidth * texHeight * chanels);

		stbi_image_free(pixels);

		return {imageData, texWidth, texHeight};
	}
}

Scene::Scene(const std::string& name) :
    mName{name},
    mSceneMeshes(),
    mStaticMeshBoundingVolume(),
    mDynamicMeshBoundingVolume(),
	mSceneAABB(float3(std::numeric_limits<float>::max()), float3(std::numeric_limits<float>::min())),
	mSceneCamera(float3(), float3(0.0f, 0.0f, 1.0f), 0.1f, 2000.0f),
	mFinalised(false),
	mMaterials{},
	mMaterialImageViews{},
    mLights{},
    mShadowingLight{},
	mSkybox{nullptr}
{
}


Scene::Scene(Scene&& scene) :
    mName{std::move(scene.mName)},
    mSceneMeshes{std::move(scene.mSceneMeshes)},
    mStaticMeshInstances{std::move(scene.mStaticMeshInstances)},
    mDynamicMeshInstances{std::move(scene.mDynamicMeshInstances)},
    mStaticMeshBoundingVolume{std::move(scene.mStaticMeshBoundingVolume)},
    mDynamicMeshBoundingVolume{std::move(scene.mDynamicMeshBoundingVolume)},
    mSceneAABB{std::move(scene.mSceneAABB)},
    mSceneCamera{std::move(scene.mSceneCamera)},
	mFinalised{scene.mFinalised.load(std::memory_order::memory_order_relaxed)},
    mMaterials{std::move(scene.mMaterials)},
    mMaterialImageViews{std::move(scene.mMaterialImageViews)},
    mLights{std::move(scene.mLights)},
    mShadowingLight{std::move(scene.mShadowingLight)},
    mSkybox{std::move(scene.mSkybox)},
    mSkyboxView(std::move(scene.mSkyboxView))
{
}


Scene& Scene::operator=(Scene&& scene)
{
    mName = std::move(scene.mName);
    mSceneMeshes = std::move(scene.mSceneMeshes);
    mStaticMeshInstances = std::move(scene.mStaticMeshInstances);
    mDynamicMeshInstances = std::move(scene.mDynamicMeshInstances);
    mStaticMeshBoundingVolume = std::move(scene.mStaticMeshBoundingVolume);
    mDynamicMeshBoundingVolume = std::move(scene.mDynamicMeshBoundingVolume);
    mSceneAABB = std::move(scene.mSceneAABB);
    mSceneCamera = std::move(scene.mSceneCamera);
    mFinalised = scene.mFinalised.load(std::memory_order::memory_order_relaxed);
	mMaterials = std::move(scene.mMaterials);
	mMaterialImageViews = std::move(scene.mMaterialImageViews);
    mLights = std::move(scene.mLights);
    mShadowingLight = std::move(scene.mShadowingLight);
	mSkybox = std::move(scene.mSkybox);
    mSkyboxView = std::move(scene.mSkyboxView);

    return *this;
}


void Scene::loadSkybox(const std::array<std::string, 6>& paths, Engine* eng)
{
    mSkybox = std::make_unique<Image>(eng->getDevice(), Format::RGBA8SRGB, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::TransferDest,
                               512, 512, 1, 1, 6, 1, "Skybox");

    mSkyboxView = std::make_unique<ImageView>(*mSkybox, ImageViewType::Colour, 0, 6);

	uint32_t i = 0;
	for(const std::string& file : paths)
	{
        TextureInfo info = loadTexture(file.c_str(), STBI_rgb_alpha);
		(*mSkybox)->setContents(info.mData.data(), 512, 512, 1, i);
		++i;
	}
}


void Scene::loadFromFile(const int vertAttributes, Engine* eng)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(mName.string().c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_GenNormals |
                                             aiProcess_FlipUVs);

    const aiNode* rootNode = scene->mRootNode;

	// Bit of a hack to avoid use after free when resizing the mesh vector
	// so just reserve enough sie here.
	mSceneMeshes.reserve(scene->mNumMeshes);

	const auto meshMaterials = loadMaterials(eng);

	// parse node is recursive so will add all meshes to the scene.
	parseNode(scene, rootNode, aiMatrix4x4{}, vertAttributes, meshMaterials);

    addLights(scene);

    // generate the BVH.
    finalise();
}


void Scene::parseNode(const aiScene* scene,
					  const aiNode* node,
					  const aiMatrix4x4& parentTransofrmation,
					  const int vertAttributes,
					  const MaterialMappings& materialIndexMappings)
{
    aiMatrix4x4 transformation = parentTransofrmation * node->mTransformation;

    for(uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* currentMesh = scene->mMeshes[node->mMeshes[i]];

        const auto nameIt = materialIndexMappings.find(currentMesh->mName);
        BELL_ASSERT(nameIt != materialIndexMappings.end(), "Unabel to find mesh material index")

        const uint32_t materialIndex = nameIt->second;

		StaticMesh mesh{currentMesh, vertAttributes, materialIndex};

        const SceneID meshID = addMesh(mesh, MeshType::Static);

        const glm::mat4 transformationMatrix{transformation.a1, transformation.a2, transformation.a3, transformation.a4,
                                             transformation.b1, transformation.b2, transformation.b3, transformation.b4,
                                             transformation.c1, transformation.c2, transformation.c3, transformation.c4,
                                             transformation.d1, transformation.d2, transformation.d3, transformation.d4};

        // TODO: For now don't attempt to deduplicate the meshes (even though this class has the functionality)
        addMeshInstance(meshID, transformationMatrix);
    }

    // Recurse through all child nodes
    for(uint32_t i = 0; i < node->mNumChildren; ++i)
    {
		parseNode(scene, node->mChildren[i], transformation, vertAttributes, materialIndexMappings);
    }
}


void Scene::addLights(const aiScene* scene)
{
    mLights.reserve(scene->mNumLights);

    for (uint32_t i = 0; i < scene->mNumLights; ++i)
    {
        const aiLight* light = scene->mLights[i];

        Light sceneLight;

        sceneLight.mType = [&]()
        {
            switch (light->mType)
            {
            case aiLightSource_POINT:
                return LightType::Point;

            case aiLightSource_SPOT:
                return LightType::Spot;

            case aiLightSource_AREA:
                return LightType::Area;

            default:
                return LightType::Point;
            }
        }();

        sceneLight.mInfluence = light->mAttenuationLinear;
        sceneLight.mPosition = float4(light->mPosition.x, light->mPosition.y, light->mPosition.z, 1.0f);
        sceneLight.mDirection = float4(light->mDirection.x, light->mDirection.y, light->mDirection.z, 1.0f);
        sceneLight.mALbedo = float4(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b, 1.0f);

        mLights.push_back(sceneLight);
    }
}


Scene::MaterialMappings Scene::loadMaterials(Engine* eng)
{
	// TODO replace this with a lower level file interface to avoid horrible iostream performance.
	std::ifstream materialFile{};
    materialFile.open(mName.concat(".mat"), std::ios::in);

	std::filesystem::path sceneDirectory = mName.parent_path();

	MaterialMappings materialMappings;

	std::string token;
	uint32_t materialIndex;
	uint32_t numMeshes;

	materialFile >> numMeshes;

	for(uint32_t i = 0; i < numMeshes; ++i)
	{
		materialFile >> token;
		materialFile >> materialIndex;

		materialMappings.insert({aiString(token), materialIndex});
	}

	std::string albedoFile;
	std::string normalsFile;
	std::string roughnessFile;
	std::string metalnessFile;
	while(materialFile >> token)
	{
		if(token == "Material")
		{
			materialFile >> materialIndex;
			continue;
		}
		else if(token == "Albedo")
		{
			materialFile >> albedoFile;
		}
		else if(token == "Normal")
		{
			materialFile >> normalsFile;
		}
		else if(token == "Roughness")
		{
			materialFile >> roughnessFile;
		}
		else if(token == "Metalness")
		{
			materialFile >> metalnessFile;

			TextureInfo diffuseInfo = loadTexture((sceneDirectory / albedoFile).string().c_str(), STBI_rgb_alpha);
            Image diffuseTexture(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1, 1, 1, 1, albedoFile);
			diffuseTexture->setContents(diffuseInfo.mData.data(), static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1);


			TextureInfo normalsInfo = loadTexture((sceneDirectory / normalsFile).string().c_str(), STBI_rgb_alpha);
            Image normalsTexture(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1, 1, 1, 1, normalsFile);
			normalsTexture->setContents(normalsInfo.mData.data(), static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1);


            TextureInfo roughnessInfo = loadTexture((sceneDirectory / roughnessFile).string().c_str(), STBI_grey);
            Image roughnessTexture(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1, 1, 1, 1, roughnessFile);
			roughnessTexture->setContents(roughnessInfo.mData.data(), static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1);


            TextureInfo metalnessInfo = loadTexture((sceneDirectory / metalnessFile).string().c_str(), STBI_grey);
            Image metalnessTexture(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(metalnessInfo.width), static_cast<uint32_t>(metalnessInfo.height), 1, 1, 1, 1, metalnessFile);
			metalnessTexture->setContents(metalnessInfo.mData.data(), static_cast<uint32_t>(metalnessInfo.width), static_cast<uint32_t>(metalnessInfo.height), 1);

			mMaterials.push_back({diffuseTexture, normalsTexture, roughnessTexture, metalnessTexture});

			mMaterialImageViews.emplace_back(diffuseTexture, ImageViewType::Colour);
			mMaterialImageViews.emplace_back(normalsTexture, ImageViewType::Colour);
			mMaterialImageViews.emplace_back(roughnessTexture, ImageViewType::Colour);
			mMaterialImageViews.emplace_back(metalnessTexture, ImageViewType::Colour);
		}
		else
		{
			BELL_LOG("unrecognised token in material file")
			BELL_TRAP;
		}
	}

	return materialMappings;
}


SceneID Scene::addMesh(const StaticMesh& mesh, MeshType meshType)
{
    SceneID id = mSceneMeshes.size();

    mSceneMeshes.push_back({mesh, meshType});

    return id;
}


// It's invalid to use the InstanceID for a static mesh for anything other than state tracking.
// As the BVH for them will not be updated.
InstanceID Scene::addMeshInstance(const SceneID meshID, const glm::mat4& transformation)
{
    auto& [mesh, meshType] = mSceneMeshes[meshID];

    InstanceID id = 0;

    if(meshType == MeshType::Static)
    {
        id = static_cast<InstanceID>(mStaticMeshInstances.size() + 1);

        mStaticMeshInstances.push_back({&mesh, transformation});
    }
    else
    {
        id = -static_cast<InstanceID>(mDynamicMeshInstances.size() - 1);

        mDynamicMeshInstances.push_back({&mesh, transformation});
    }

    return id;
}


void Scene::finalise()
{
    const bool firstTime = !mFinalised.load(std::memory_order::memory_order_relaxed);

    mFinalised.store(true);

    generateSceneAABB(firstTime);

    if(firstTime)
    {
        //Build the static meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> staticBVHMeshes{};

        std::transform(mStaticMeshInstances.begin(), mStaticMeshInstances.end(), std::back_inserter(staticBVHMeshes),
                       [](MeshInstance& instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance.mMesh->getAABB() * instance.mTransformation, &instance}; } );

        OctTreeFactory<MeshInstance*> staticBVHFactory(mSceneAABB, staticBVHMeshes);

        mStaticMeshBoundingVolume = staticBVHFactory.generateOctTree(3);
    }

    //Build the dynamic meshes BVH structure.
    std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> dynamicBVHMeshes{};

    std::transform(mDynamicMeshInstances.begin(), mDynamicMeshInstances.end(), std::back_inserter(dynamicBVHMeshes),
                   [](MeshInstance& instance)
                    { return OctTreeFactory<MeshInstance*>::BuilderNode{instance.mMesh->getAABB() * instance.mTransformation, &instance}; } );

    // Not all scenes contain dynamic meshes.
    if(!dynamicBVHMeshes.empty())
    {
        OctTreeFactory<MeshInstance*> dynamicBVHFactory(mSceneAABB, dynamicBVHMeshes);

        mStaticMeshBoundingVolume = dynamicBVHFactory.generateOctTree(3);
    }
}


std::vector<const Scene::MeshInstance *> Scene::getViewableMeshes() const
{
    std::vector<const MeshInstance*> instances;

    Frustum currentFrustum = mSceneCamera.getFrustum();

	std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.containedWithin(currentFrustum, EstimationMode::Over);
    std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.containedWithin(currentFrustum, EstimationMode::Over);

    instances.insert(instances.end(), staticMeshes.begin(), staticMeshes.end());
    instances.insert(instances.end(), dynamicMeshes.begin(), dynamicMeshes.end());

    return instances;
}


Scene::MeshInstance* Scene::getMeshInstance(const InstanceID id)
{
    BELL_ASSERT(id < 0, "Trying to fetch a static mesh, altering this after scene creation is undefined")

    uint64_t meshIndex = -(id + 1);

    return &mDynamicMeshInstances[meshIndex];
}


// Find the smallest and largest coords amoung the meshes AABB and set the scene AABB to those.
void Scene::generateSceneAABB(const bool includeStatic)
{
    Cube sceneCube = mSceneAABB.getCube();

    if(includeStatic)
    {
        for(const auto& instance : mStaticMeshInstances)
        {
            AABB meshAABB = instance.mMesh->getAABB() * instance.mTransformation;
            Cube instanceCube = meshAABB.getCube();

            sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

            sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
        }
    }

    for(const auto& instance : mDynamicMeshInstances)
    {
        AABB meshAABB = instance.mMesh->getAABB() * instance.mTransformation;
        Cube instanceCube = meshAABB.getCube();

        sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

        sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
    }

    mSceneAABB = AABB(sceneCube);
}


void Scene::setShadowingLight(const glm::vec3& position, const glm::vec3& direction)
{
    const glm::mat4 view = glm::lookAt(position, position + direction, glm::vec3(0.0f, -1.0f, 0.0f));
    const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 2000.0f);

    ShadowingLight light{view, glm::inverse(view), proj * view};
    mShadowingLight = light;
}

