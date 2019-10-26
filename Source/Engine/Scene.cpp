#include "Engine/Scene.h"
#include "Engine/Engine.hpp"
#include "Core/BellLogging.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "stb_image.h"

#include <algorithm>
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

	TextureInfo loadTexture(const char* filePath)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		std::vector<unsigned char> imageData{};
		imageData.resize(texWidth * texHeight * 4);

		std::memcpy(imageData.data(), pixels, texWidth * texHeight * 4);

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
	mSceneCamera(float3(), float3(0.0f, 0.0f, 1.0f)),
    mFinalised(false)
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
    mFinalised{scene.mFinalised.load(std::memory_order::memory_order_relaxed)}
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

    return *this;
}


void Scene::loadFromFile(const int vertAttributes, Engine* eng)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(mName.c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_GenNormals |
                                             aiProcess_FlipUVs);

    const aiNode* rootNode = scene->mRootNode;

	// Bit of a hack to avoid use after free when resizing the mesh vector
	// so just reserve enough sie here.
	mSceneMeshes.reserve(scene->mNumMeshes);

	// parse node is recursive so will add all meshes to the scene.
    parseNode(scene, rootNode, aiMatrix4x4{}, vertAttributes);

	// Load all the materials referenced by the scene.
	parseMaterials(scene, eng);

    // generate the BVH.
    finalise();
}


void Scene::parseNode(const aiScene* scene,
                      const aiNode* node,
                      const aiMatrix4x4& parentTransofrmation,
                      const int vertAttributes)
{
    aiMatrix4x4 transformation = parentTransofrmation * node->mTransformation;

    for(uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* currentMesh = scene->mMeshes[node->mMeshes[i]];
        StaticMesh mesh{currentMesh, vertAttributes};

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
        parseNode(scene, node->mChildren[i], transformation, vertAttributes);
    }
}


void Scene::parseMaterials(const aiScene* scene, Engine* eng)
{
	mMaterials.reserve(scene->mNumMaterials);
	mMaterialImageViews.reserve(scene->mNumMaterials * 4);

	for(uint32_t i = 0; i < scene->mNumMaterials; ++i)
	{
		aiString diffuseFilePath;
		scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_DIFFUSE, 0, &diffuseFilePath);

		aiString normalFilePath;
		scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &normalFilePath);

		// Assimp doesn't support metalness and roughness properly so load two unknown textures.
		aiString roughnessFilePath;
		scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_UNKNOWN, 0, &roughnessFilePath);

		aiString metalnessFilePath;
		scene->mMaterials[i]->GetTexture(aiTextureType::aiTextureType_UNKNOWN, 1, &metalnessFilePath);

		TextureInfo diffuseInfo = loadTexture(diffuseFilePath.C_Str());
		Image diffuseTexture(eng->getDevice(), Format::RGBA8SRGB, ImageUsage::Sampled | ImageUsage::TransferDest,
							 static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1);
		diffuseTexture.setContents(diffuseInfo.mData.data(), static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1);


		TextureInfo normalsInfo = loadTexture(diffuseFilePath.C_Str());
		Image normalsTexture(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
							 static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1);
		normalsTexture.setContents(normalsInfo.mData.data(), static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1);


		TextureInfo roughnessInfo = loadTexture(diffuseFilePath.C_Str());
		Image roughnessTexture(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
							 static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1);
		roughnessTexture.setContents(roughnessInfo.mData.data(), static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1);


		TextureInfo metalnessInfo = loadTexture(diffuseFilePath.C_Str());
		Image metalnessTexture(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
							 static_cast<uint32_t>(metalnessInfo.width), static_cast<uint32_t>(metalnessInfo.height), 1);
		metalnessTexture.setContents(metalnessInfo.mData.data(), static_cast<uint32_t>(metalnessInfo.width), static_cast<uint32_t>(metalnessInfo.height), 1);

		mMaterials.push_back({diffuseTexture, normalsTexture, roughnessTexture, metalnessTexture});

		mMaterialImageViews.emplace_back(diffuseTexture, ImageViewType::Colour);
		mMaterialImageViews.emplace_back(normalsTexture, ImageViewType::Colour);
		mMaterialImageViews.emplace_back(roughnessTexture, ImageViewType::Colour);
		mMaterialImageViews.emplace_back(metalnessTexture, ImageViewType::Colour);
	}
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
        std::vector<typename BVHFactory<MeshInstance*>::BuilderNode> staticBVHMeshes{};

        std::transform(mStaticMeshInstances.begin(), mStaticMeshInstances.end(), std::back_inserter(staticBVHMeshes),
                       [](MeshInstance& instance)
                        { return BVHFactory<MeshInstance*>::BuilderNode{instance.mMesh->getAABB() * instance.mTransformation, &instance}; } );

        BVHFactory<MeshInstance*> staticBVHFactory(mSceneAABB, staticBVHMeshes);

        mStaticMeshBoundingVolume = staticBVHFactory.generateBVH();
    }

    //Build the dynamic meshes BVH structure.
    std::vector<typename BVHFactory<MeshInstance*>::BuilderNode> dynamicBVHMeshes{};

    std::transform(mDynamicMeshInstances.begin(), mDynamicMeshInstances.end(), std::back_inserter(dynamicBVHMeshes),
                   [](MeshInstance& instance)
                    { return BVHFactory<MeshInstance*>::BuilderNode{instance.mMesh->getAABB() * instance.mTransformation, &instance}; } );

    // Not all scenes contain dynamic meshes.
    if(!dynamicBVHMeshes.empty())
    {
        BVHFactory<MeshInstance*> dynamicBVHFactory(mSceneAABB, dynamicBVHMeshes);

        mStaticMeshBoundingVolume = dynamicBVHFactory.generateBVH();
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

