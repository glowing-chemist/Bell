#include "Engine/Scene.h"
#include "Engine/Engine.hpp"
#include "Engine/TextureUtil.hpp"
#include "Core/BellLogging.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>


Scene::Scene(const std::string& name) :
    mName{name},
    mSceneMeshes(),
    mStaticMeshBoundingVolume(),
    mDynamicMeshBoundingVolume(),
	mSceneAABB(float4(std::numeric_limits<float>::max()), float4(std::numeric_limits<float>::min())),
	mSceneCamera(float3(), float3(0.0f, 0.0f, 1.0f), 1920.0f /1080.0f ,0.1f, 2000.0f),
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
	uint32_t i = 0;
	for(const std::string& file : paths)
	{
        TextureUtil::TextureInfo info = TextureUtil::load32BitTexture(file.c_str(), STBI_rgb_alpha);

		if (i == 0)
		{
			mSkybox = std::make_unique<Image>(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::TransferDest,
				info.width, info.height, 1, 1, 6, 1, "Skybox");

			mSkyboxView = std::make_unique<ImageView>(*mSkybox, ImageViewType::CubeMap, 0, 6);
		}

		(*mSkybox)->setContents(info.mData.data(), info.width, info.height, 1, i);
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

	const auto meshMaterials = loadMaterialsInternal(eng);

	// parse node is recursive so will add all meshes to the scene.
	parseNode(scene, rootNode, aiMatrix4x4{}, vertAttributes, meshMaterials);

    addLights(scene);
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

        const uint32_t materialIndex = nameIt->second.index;

        StaticMesh mesh{currentMesh, vertAttributes};
        mesh.setAttributes(nameIt->second.attributes);

        const SceneID meshID = addMesh(mesh, MeshType::Static);

        const glm::mat4 transformationMatrix{transformation.a1, transformation.a2, transformation.a3, transformation.a4,
                                             transformation.b1, transformation.b2, transformation.b3, transformation.b4,
                                             transformation.c1, transformation.c2, transformation.c3, transformation.c4,
                                             transformation.d1, transformation.d2, transformation.d3, transformation.d4};

        // TODO: For now don't attempt to deduplicate the meshes (even though this class has the functionality)
        addMeshInstance(meshID, transformationMatrix, materialIndex);
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

        sceneLight.mRadius = light->mAttenuationLinear;
		sceneLight.mIntensity= light->mAttenuationLinear;
        sceneLight.mPosition = float4(light->mPosition.x, light->mPosition.y, light->mPosition.z, 1.0f);
        sceneLight.mDirection = float4(light->mDirection.x, light->mDirection.y, light->mDirection.z, 1.0f);
        sceneLight.mAlbedo = float4(light->mColorDiffuse.r, light->mColorDiffuse.g, light->mColorDiffuse.b, 1.0f);
        sceneLight.mUp = float4(light->mUp.x, light->mUp.y, light->mUp.z, 1.0f);
        sceneLight.mAngleSize = 45.0f;

        mLights.push_back(sceneLight);
    }
}


void Scene::loadMaterials(Engine* eng)
{
	const MaterialMappings mappings = loadMaterialsInternal(eng);
}


Scene::MaterialMappings Scene::loadMaterialsInternal(Engine* eng)
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

        uint32_t attributes = 0;
        std::string attributeName;
        while(materialFile >> attributeName)
        {
            if(attributeName == ";")
                break;
            else if(attributeName == "AlphaTested")
                attributes |= MeshAttributes::AlphaTested;
            else if(attributeName == "Transparent")
                attributes |= MeshAttributes::Transparent;
        }

        materialMappings.insert({aiString(token), {materialIndex, attributes}});
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

            TextureUtil::TextureInfo diffuseInfo = TextureUtil::load32BitTexture((sceneDirectory / albedoFile).string().c_str(), STBI_rgb_alpha);
            Image diffuseTexture(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1, 1, 1, 1, albedoFile);
			diffuseTexture->setContents(diffuseInfo.mData.data(), static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1);


            TextureUtil::TextureInfo normalsInfo = TextureUtil::load32BitTexture((sceneDirectory / normalsFile).string().c_str(), STBI_rgb_alpha);
            Image normalsTexture(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1, 1, 1, 1, normalsFile);
			normalsTexture->setContents(normalsInfo.mData.data(), static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1);


            TextureUtil::TextureInfo roughnessInfo = TextureUtil::load32BitTexture((sceneDirectory / roughnessFile).string().c_str(), STBI_grey);
            Image roughnessTexture(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest,
								 static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1, 1, 1, 1, roughnessFile);
			roughnessTexture->setContents(roughnessInfo.mData.data(), static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1);


            TextureUtil::TextureInfo metalnessInfo = TextureUtil::load32BitTexture((sceneDirectory / metalnessFile).string().c_str(), STBI_grey);
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
InstanceID Scene::addMeshInstance(const SceneID meshID, const float4x3 &transformation, const uint32_t materialID)
{
    auto& [mesh, meshType] = mSceneMeshes[meshID];

    InstanceID id = 0;

    if(meshType == MeshType::Static)
    {
        id = static_cast<InstanceID>(mStaticMeshInstances.size() + 1);

        mStaticMeshInstances.push_back({&mesh, transformation, materialID});
    }
    else
    {
        id = -static_cast<InstanceID>(mDynamicMeshInstances.size());

        mDynamicMeshInstances.push_back({&mesh, transformation, materialID});
    }

    return id;
}


void Scene::uploadData(Engine* eng)
{
    eng->flushWait();
    eng->clearVertexCache();

    for(const auto& [mesh, meshType] : mSceneMeshes)
    {
        eng->addMeshToBuffer(&mesh);
    }
}


void Scene::computeBounds(const MeshType type)
{
    generateSceneAABB(type == MeshType::Static);

    if(type == MeshType::Static)
    {
        //Build the static meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> staticBVHMeshes{};

        std::transform(mStaticMeshInstances.begin(), mStaticMeshInstances.end(), std::back_inserter(staticBVHMeshes),
                       [](MeshInstance& instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance.mMesh->getAABB() * instance.getTransMatrix(), &instance}; } );

        OctTreeFactory<MeshInstance*> staticBVHFactory(mSceneAABB, staticBVHMeshes);

        mStaticMeshBoundingVolume = staticBVHFactory.generateOctTree(3);
    }
    else
    {
        //Build the dynamic meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> dynamicBVHMeshes{};

        std::transform(mDynamicMeshInstances.begin(), mDynamicMeshInstances.end(), std::back_inserter(dynamicBVHMeshes),
                       [](MeshInstance& instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance.mMesh->getAABB() * instance.getTransMatrix(), &instance}; } );


         OctTreeFactory<MeshInstance*> dynamicBVHFactory(mSceneAABB, dynamicBVHMeshes);

         mDynamicMeshBoundingVolume = dynamicBVHFactory.generateOctTree(3);
    }
}


std::vector<const MeshInstance *> Scene::getViewableMeshes(const Frustum& frustum) const
{
    std::vector<const MeshInstance*> instances;

    std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.containedWithin(frustum, EstimationMode::Over);
    std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.containedWithin(frustum, EstimationMode::Over);

    instances.insert(instances.end(), staticMeshes.begin(), staticMeshes.end());
    instances.insert(instances.end(), dynamicMeshes.begin(), dynamicMeshes.end());

    return instances;
}


Frustum Scene::getShadowingLightFrustum() const
{
    return Frustum(mShadowingLight.mPosition, mShadowingLight.mDirection, mShadowingLight.mUp, 2000.0f, 0.1f, 90.0f, 1920.0f / 1080.0f);
}


MeshInstance* Scene::getMeshInstance(const InstanceID id)
{
	if (id <= 0)
	{
		uint64_t meshIndex = -id;

		return &mDynamicMeshInstances[meshIndex];
	}
	else
	{
		uint64_t meshIndex = id - 1;

		return &mStaticMeshInstances[meshIndex];
	}
}


// Find the smallest and largest coords amoung the meshes AABB and set the scene AABB to those.
void Scene::generateSceneAABB(const bool includeStatic)
{
    Cube sceneCube = mSceneAABB.getCube();

    if(includeStatic)
    {
        for(const auto& instance : mStaticMeshInstances)
        {
            AABB meshAABB = instance.mMesh->getAABB() * instance.getTransMatrix();
            Cube instanceCube = meshAABB.getCube();

            sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

            sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
        }
    }

    for(const auto& instance : mDynamicMeshInstances)
    {
        AABB meshAABB = instance.mMesh->getAABB() * instance.getTransMatrix();
        Cube instanceCube = meshAABB.getCube();

        sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

        sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
    }

    mSceneAABB = AABB(sceneCube);
}


void Scene::setShadowingLight(const float3 &position, const float3 &direction, const float3 &up)
{
    const float4x4 view = glm::lookAt(position, position + direction, up);
    const float4x4 proj = glm::perspective(glm::radians(90.0f), 1920.0f / 1080.0f, 0.1f, 2000.0f);

    ShadowingLight light{view, glm::inverse(view), proj * view, float4(position, 1.0f), float4(direction, 0.0f), float4(up, 1.0f)};
    mShadowingLight = light;
}


std::vector<Scene::Intersection> Scene::getIntersections() const
{
	std::vector<Intersection> intersections;

	for (auto& mesh : mStaticMeshInstances)
	{
        const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(mesh.mMesh->getAABB() * mesh.getTransMatrix());
        const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(mesh.mMesh->getAABB() * mesh.getTransMatrix());

		for (auto& m : staticMeshes)
		{
			intersections.emplace_back(m, &mesh);
		}

		for (auto& m : dynamicMeshes)
		{
			intersections.emplace_back(m, &mesh);
		}
	}

	for (auto& mesh : mDynamicMeshInstances)
	{
        const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(mesh.mMesh->getAABB() * mesh.getTransMatrix());
        const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(mesh.mMesh->getAABB() * mesh.getTransMatrix());

		for (auto& m : staticMeshes)
		{
			intersections.emplace_back(m, &mesh);
		}

		for (auto& m : dynamicMeshes)
		{
			intersections.emplace_back(m, &mesh);
		}
	}

	return intersections;
}


std::vector<Scene::Intersection> Scene::getIntersections(const InstanceID id)
{
	const MeshInstance* instanceToTest = getMeshInstance(id);

    const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(instanceToTest->mMesh->getAABB() * instanceToTest->getTransMatrix());
    const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(instanceToTest->mMesh->getAABB() * instanceToTest->getTransMatrix());

	std::vector<Intersection> intersections{};
	intersections.reserve(staticMeshes.size() + dynamicMeshes.size());

	for (auto& m : staticMeshes)
	{
		if (m != instanceToTest)
		{
			intersections.emplace_back(instanceToTest, m);
		}
	}

	for (auto& m : dynamicMeshes)
	{
		if (m != instanceToTest)
		{
			intersections.emplace_back(instanceToTest, m);
		}
	}

	return intersections;
}


std::vector<Scene::Intersection> Scene::getIntersections(const InstanceID IgnoreID, const AABB& aabbToTest)
{
	const MeshInstance* instanceToIgnore = getMeshInstance(IgnoreID);

	const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(aabbToTest);
	const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(aabbToTest);

	std::vector<Intersection> intersections{};
	intersections.reserve(staticMeshes.size() + dynamicMeshes.size());

	for (auto& m : staticMeshes)
	{
		if (m != instanceToIgnore)
		{
			intersections.emplace_back(instanceToIgnore, m);
		}
	}

	for (auto& m : dynamicMeshes)
	{
		if (m != instanceToIgnore)
		{
			intersections.emplace_back(instanceToIgnore, m);
		}
	}

	return intersections;
}


Scene::Light Scene::Light::pointLight(const float4& position, const float4& albedo, const float intensity, const float radius)
{
    Scene::Light light{};
    light.mPosition = position;
    light.mAlbedo = albedo;
    light.mIntensity = intensity;
    light.mRadius = radius;
    light.mType = LightType::Point;

    return light;
}


Scene::Light Scene::Light::spotLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float angle)
{
    Scene::Light light{};
    light.mPosition = position;
    light.mDirection = direction;
    light.mAlbedo = albedo;
    light.mIntensity = intensity;
    light.mRadius = radius;
    light.mType = LightType::Spot;
    light.mAngleSize = angle;

    return light;
}


Scene::Light Scene::Light::areaLight(const float4& position, const float4& direction, const float4& up, const float4& albedo, const float intensity, const float radius, const float size)
{
    Scene::Light light{};
    light.mPosition = position;
    light.mDirection = direction;
    light.mUp = up;
    light.mAlbedo = albedo;
    light.mIntensity = intensity;
    light.mType = LightType::Area;
    light.mRadius = radius;
    light.mAngleSize = size;


    return light;
}


Scene::Light Scene::Light::stripLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float size)
{
    Scene::Light light{};
    light.mPosition = position;
    light.mDirection = direction;
    light.mAlbedo = albedo;
    light.mIntensity = intensity;
    light.mType = LightType::Strip;
    light.mRadius = radius;
    light.mAngleSize = size;

    return light;
}
