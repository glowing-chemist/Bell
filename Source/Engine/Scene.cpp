#include "Engine/Scene.h"
#include "Engine/Engine.hpp"
#include "Engine/TextureUtil.hpp"
#include "Core/BellLogging.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"

#include "glm/gtc/matrix_transform.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>


StaticMesh* MeshInstance::getMesh()
{
    return mScene->getMesh(mMesh);
}

const StaticMesh* MeshInstance::getMesh() const
{
    return mScene->getMesh(mMesh);
}


Scene::Scene(const std::filesystem::path& path) :
    mPath{path},
    mSceneMeshes(),
    mStaticMeshBoundingVolume(),
    mDynamicMeshBoundingVolume(),
	mSceneAABB(float4(std::numeric_limits<float>::max()), float4(std::numeric_limits<float>::min())),
    mSceneCamera(float3(0.0f, 0.0f, 0.0f), float3(0.0f, 0.0f, 1.0f), 1920.0f /1080.0f ,0.1f, 2000.0f),
	mMaterials{},
	mMaterialImageViews{},
    mLights{},
    mShadowLightCamera(Camera({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.0f)),
    mShadowingLight{},
    mCascadesInfo{mSceneCamera.getFarPlane() * 0.3f, mSceneCamera.getFarPlane() * 0.7f, mSceneCamera.getFarPlane()},
	mSkybox{nullptr}
{
    setShadowingLight(Camera({0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, 0.0f));
}


Scene::~Scene()
{
    for(const auto& mat : mMaterials)
    {
        delete mat.mNormals;
        delete mat.mAlbedoorDiffuse;
        delete mat.mRoughnessOrGloss;
        delete mat.mMetalnessOrSpecular;
        delete mat.mEmissive;
        delete mat.mAmbientOcclusion;
    }
}


void Scene::loadSkybox(const std::array<std::string, 6>& paths, Engine* eng)
{
	uint32_t i = 0;
    std::vector<unsigned char> skyboxData{};
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
        skyboxData.insert(skyboxData.end(), info.mData.begin(), info.mData.end());
		++i;
	}

    // create CPU skybox.
    ImageExtent extent = (*mSkybox)->getExtent(0, 0);
    extent.depth = 6;
    mCPUSkybox = std::make_unique<CPUImage>(std::move(skyboxData), extent, Format::RGBA8UNorm);
}


std::vector<InstanceID> Scene::loadFromFile(const int vertAttributes, Engine* eng)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(mPath.string().c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_GenNormals |
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_FlipUVs);

    const aiNode* rootNode = scene->mRootNode;

	mSceneMeshes.reserve(scene->mNumMeshes);


    MaterialMappings meshMaterials;
    fs::path materialFile{mPath};
    materialFile += ".mat";
    if(fs::exists(materialFile))
        meshMaterials = loadMaterialsInternal(eng); // Load materials from the internal .mat format.
    else
        loadMaterialsExternal(eng, scene);

    std::unordered_map<const aiMesh*, SceneID> meshToSceneIDMapping{};

    std::vector<InstanceID> instanceIDs;

	// parse node is recursive so will add all meshes to the scene.
    parseNode(scene,
              rootNode,
              aiMatrix4x4{},
              kInvalidInstanceID,
              vertAttributes,
              meshMaterials,
              meshToSceneIDMapping,
              instanceIDs);

    addLights(scene);

    return instanceIDs;
}


void Scene::parseNode(const aiScene* scene,
                      const aiNode* node,
                      const aiMatrix4x4& parentTransofrmation,
                      const InstanceID parentID,
                      const int vertAttributes,
                      const MaterialMappings& materialIndexMappings,
                      std::unordered_map<const aiMesh*, SceneID>& meshMappings,
                      std::vector<InstanceID>& instanceIds)
{
    aiMatrix4x4 transformation = parentTransofrmation * node->mTransformation;

    InstanceID currentInstanceID = parentID;
    for(uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* currentMesh = scene->mMeshes[node->mMeshes[i]];
        uint32_t materialIndex = ~0;

        const auto nameIt = materialIndexMappings.find(currentMesh->mName);
        if(nameIt != materialIndexMappings.end()) // Material info has already been parsed from separate material file.
        {
            materialIndex = nameIt->second;
        }
        else // No material file found, so get material index from the mesh file.
        {
            materialIndex = currentMesh->mMaterialIndex;
        }

        const uint32_t materialOffset = mMaterials[materialIndex].mMaterialOffset;
        const uint32_t materialFlags = mMaterials[materialIndex].mMaterialTypes;

        SceneID meshID = 0;
        if(meshMappings.find(currentMesh) == meshMappings.end())
        {
            StaticMesh mesh{scene, currentMesh, vertAttributes};

            meshID = addMesh(mesh, MeshType::Static);

            meshMappings[currentMesh] = meshID;
        }
        else
            meshID = meshMappings[currentMesh];

        float4x4 transformationMatrix{};
        transformationMatrix[0][0] = transformation.a1; transformationMatrix[0][1] = transformation.b1;  transformationMatrix[0][2] = transformation.c1; transformationMatrix[0][3] = transformation.d1;
        transformationMatrix[1][0] = transformation.a2; transformationMatrix[1][1] = transformation.b2;  transformationMatrix[1][2] = transformation.c2; transformationMatrix[1][3] = transformation.d2;
        transformationMatrix[2][0] = transformation.a3; transformationMatrix[2][1] = transformation.b3;  transformationMatrix[2][2] = transformation.c3; transformationMatrix[2][3] = transformation.d3;
        transformationMatrix[3][0] = transformation.a4; transformationMatrix[3][1] = transformation.b4;  transformationMatrix[3][2] = transformation.c4; transformationMatrix[3][3] = transformation.d4;


        InstanceID instanceID = addMeshInstance(meshID, parentID, transformationMatrix, materialOffset, materialFlags, currentMesh->mName.C_Str());
        currentInstanceID = instanceID;
        instanceIds.push_back(instanceID);
    }

    // Recurse through all child nodes
    for(uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        parseNode(scene,
                  node->mChildren[i],
                  currentInstanceID == kInvalidInstanceID ? transformation : aiMatrix4x4{},
                  currentInstanceID,
                  vertAttributes,
                  materialIndexMappings,
                  meshMappings,
                  instanceIds);
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
    materialFile.open(mPath.concat(".mat"), std::ios::in);

    std::filesystem::path sceneDirectory = mPath.parent_path();

	MaterialMappings materialMappings;

	std::string token;
    std::string delimiter;
	uint32_t materialIndex;
	uint32_t numMeshes;

	materialFile >> numMeshes;

	for(uint32_t i = 0; i < numMeshes; ++i)
	{
		materialFile >> token;
		materialFile >> materialIndex;
        materialFile >> delimiter;

        materialMappings.insert({aiString(token), materialIndex});
	}

    MaterialPaths mat{"", "", "", "", "", "", "", "", 0, 0};
    std::string albedoOrDiffuseFile;
    std::string normalsFile;
    std::string roughnessOrGlossFile;
    std::string metalnessOrSpecularFile;
    std::string emissiveFile;
    std::string ambientOcclusionFile;
    std::string heightMapFile;
	while(materialFile >> token)
	{
		if(token == "Material")
		{
            mat.mMaterialOffset = mMaterialImageViews.size();

            // add the previously read material if it exists.
            if(mat.mMaterialTypes)
                addMaterial(mat, eng);

            mat.mNormalsPath = "";
            mat.mAlbedoorDiffusePath = "";
            mat.mRoughnessOrGlossPath = "";
            mat.mMetalnessOrSpecularPath = "";
            mat.mMaterialTypes = 0;

			materialFile >> materialIndex;
            mat.mName = std::to_string(materialIndex);

            std::string materialAttributes;
            while (materialFile >> materialAttributes)
            {
                if (materialAttributes == ";")
                    break;
                else if (materialAttributes == "AlphaTested")
                    mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::AlphaTested);
                else if(materialAttributes == "Transparent")
                    mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Transparent);
            }

			continue;
		}
        else if(token == "Albedo" || token == "Diffuse")
		{
            materialFile >> albedoOrDiffuseFile;
            mat.mAlbedoorDiffusePath = (sceneDirectory / albedoOrDiffuseFile).string();
            mat.mMaterialTypes |= token == "Albedo" ? static_cast<uint32_t>(MaterialType::Albedo) : static_cast<uint32_t>(MaterialType::Diffuse);
		}
		else if(token == "Normal")
		{
            materialFile >> normalsFile;
            mat.mNormalsPath = (sceneDirectory / normalsFile).string();
            mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Normals);
		}
        else if(token == "Roughness" || token == "Gloss")
		{
            materialFile >> roughnessOrGlossFile;
            mat.mRoughnessOrGlossPath = (sceneDirectory / roughnessOrGlossFile).string();
            mat.mMaterialTypes |= token == "Roughness" ? static_cast<uint32_t>(MaterialType::Roughness) : static_cast<uint32_t>(MaterialType::Gloss);
		}
        else if(token == "Metalness" || token == "Specular")
		{
            materialFile >> metalnessOrSpecularFile;
            mat.mMetalnessOrSpecularPath = (sceneDirectory / metalnessOrSpecularFile).string();
            mat.mMaterialTypes |= token == "Metalness" ? static_cast<uint32_t>(MaterialType::Metalness) : static_cast<uint32_t>(MaterialType::Specular);
		}
        else if(token == "CombinedMetalRoughness")
        {
            materialFile >> roughnessOrGlossFile;
            mat.mRoughnessOrGlossPath = (sceneDirectory / metalnessOrSpecularFile).string();
            mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::CombinedMetalnessRoughness);
        }
        else if(token == "Emissive")
        {
            materialFile >> emissiveFile;
            mat.mEmissivePath = (sceneDirectory / emissiveFile).string();
            mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Emisive);
        }
        else if(token == "AO")
        {
            materialFile >> ambientOcclusionFile;
            mat.mAmbientOcclusionPath = (sceneDirectory / ambientOcclusionFile).string();
            mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::AmbientOcclusion);
        }
        else if(token == "HeightMap")
        {
            materialFile >> heightMapFile;
            mat.mHeightMapPath = (sceneDirectory / heightMapFile).string();
            mat.mMaterialTypes |= static_cast<uint32_t>(MaterialType::HeightMap);
        }
		else
		{
			BELL_LOG("unrecognised token in material file")
			BELL_TRAP;
		}
	}
    // Add the last material
    if(mat.mMaterialTypes)
    {
        mat.mMaterialOffset = mMaterialImageViews.size();
        addMaterial(mat, eng);
    }

    return materialMappings;
}


void Scene::loadMaterialsExternal(Engine* eng, const aiScene* scene)
{
    const std::string sceneDirectory = mPath.parent_path().string();

    auto pathMapping = [](const fs::path& path) -> std::string
    {
        std::string mappedPath = path.string();
        std::replace(mappedPath.begin(), mappedPath.end(),
             #ifdef _MSC_VER
                '/', '\\'
             #else
                '\\', '/'
             #endif
                     );

        return mappedPath;
    };

    for(uint32_t i = 0; i < scene->mNumMaterials; ++i)
    {
        const aiMaterial* material = scene->mMaterials[i];

        aiString materialName;
        material->Get(AI_MATKEY_NAME, materialName);
        MaterialPaths newMaterial{materialName.C_Str(), "", "", "", "", "", "", "", 0, 0};

        if(material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_BASE_COLOR, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mAlbedoorDiffusePath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Albedo);
        }
        else if(material->GetTextureCount(aiTextureType_DIFFUSE) > 1)
        {
            aiString path;
            material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mAlbedoorDiffusePath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Albedo);
        }
        else if(material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mAlbedoorDiffusePath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Diffuse);
        }

        if(material->GetTextureCount(aiTextureType_NORMALS) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_NORMALS, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mNormalsPath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Normals);
        }

        // AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE maps to aiTextureType_UNKNOWN for gltf becuase why not.
        if(material->GetTextureCount(aiTextureType_UNKNOWN) > 0)
        {
            aiString path;
            material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mRoughnessOrGlossPath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::CombinedMetalnessRoughness);
        }
        else
        {
            if(material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
            {
                aiString path;
                material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &path);

                fs::path fsPath(path.C_Str());
                newMaterial.mAlbedoorDiffusePath = pathMapping(sceneDirectory / fsPath);
                newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Roughness);
            }
            else if(material->GetTextureCount(aiTextureType_SHININESS) > 0)
            {
                aiString path;
                material->GetTexture(aiTextureType_SHININESS, 0, &path);

                fs::path fsPath(path.C_Str());
                newMaterial.mRoughnessOrGlossPath = pathMapping(sceneDirectory / fsPath);
                newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Gloss);
            }

            if(material->GetTextureCount(aiTextureType_METALNESS) > 0)
            {
                aiString path;
                material->GetTexture(aiTextureType_METALNESS, 0, &path);

                fs::path fsPath(path.C_Str());
                newMaterial.mAlbedoorDiffusePath = pathMapping(sceneDirectory / fsPath);
                newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Metalness);
            }
            else if(material->GetTextureCount(aiTextureType_SPECULAR) > 0)
            {
                aiString path;
                material->GetTexture(aiTextureType_SPECULAR, 0, &path);

                fs::path fsPath(path.C_Str());
                newMaterial.mMetalnessOrSpecularPath = pathMapping(sceneDirectory / fsPath);

                if(material->GetTextureCount(aiTextureType_SHININESS) == 0)
                    newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::CombinedSpecularGloss);
                else
                    newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Specular);
            }
        }
        if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_EMISSIVE, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mEmissivePath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Emisive);
        }

        if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mAmbientOcclusionPath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::AmbientOcclusion);
        }
        else if(material->GetTextureCount(aiTextureType_LIGHTMAP) > 0) // Can also be an AO texture, why is there 2!?
        {
            aiString path;
            material->GetTexture(aiTextureType_LIGHTMAP, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mAmbientOcclusionPath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::AmbientOcclusion);
        }

        if(material->GetTextureCount(aiTextureType_HEIGHT) > 0)
        {
            aiString path;
            material->GetTexture(aiTextureType_HEIGHT, 0, &path);

            fs::path fsPath(path.C_Str());
            newMaterial.mHeightMapPath = pathMapping(sceneDirectory / fsPath);
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::HeightMap);
        }

        ai_real opacity = 1.0f;
        material->Get(AI_MATKEY_OPACITY, opacity);
        if(opacity < 1.0f)
            newMaterial.mMaterialTypes |= static_cast<uint32_t>(MaterialType::Transparent);

        newMaterial.mMaterialOffset = mMaterialImageViews.size();

        addMaterial(newMaterial, eng);
    }
}


SceneID Scene::addMesh(const StaticMesh& mesh, MeshType meshType)
{
    SceneID id = mSceneMeshes.size();
    mSceneMeshes.push_back({mesh, meshType});

    return id;
}


std::vector<SceneID> Scene::loadFile(const std::string &path, MeshType meshType, Engine* eng)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path.c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_GenNormals |
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_FlipUVs);

    std::vector<SceneID> ids{};
    ids.reserve(scene->mNumMeshes);
    for(uint32_t i = 0; i < scene->mNumMeshes; ++i)
    {
        const aiMesh* aimesh = scene->mMeshes[i];
        StaticMesh mesh(scene, aimesh, VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::Tangents | VertexAttributes::TextureCoordinates | VertexAttributes::Albedo);

        ids.push_back(mSceneMeshes.size());
        mSceneMeshes.push_back({mesh, meshType});
    }

    mPath = fs::path(path);
    loadMaterialsExternal(eng, scene);

    return ids;
}


// It's invalid to use the InstanceID for a static mesh for anything other than state tracking.
// As the BVH for them will not be updated.
InstanceID Scene::addMeshInstance(const SceneID meshID,
                                  const InstanceID parentInstance,
                                  const float4x4 &transformation,
                                  const uint32_t materialIndex,
                                  const uint32_t materialFlags,
                                  const std::string &name)
{
    auto& [mesh, meshType] = mSceneMeshes[meshID];

    InstanceID id = mNextInstanceID++;

    if(meshType == MeshType::Static)
    {
        if(!mFreeStaticMeshIndicies.empty())
        {
            const uint32_t freeIndex = mFreeStaticMeshIndicies.back();
            mFreeStaticMeshIndicies.pop_back();

            mInstanceMap[id] = {InstanceType::StaticMesh, freeIndex};
            mStaticMeshInstances[freeIndex] = MeshInstance{this, meshID, transformation, materialIndex, materialFlags, name};
        }
        else
        {
            mInstanceMap[id] = {InstanceType::StaticMesh, mStaticMeshInstances.size()};
            mStaticMeshInstances.push_back({this, meshID, transformation, materialIndex, materialFlags, name});
        }

        if(parentInstance != kInvalidInstanceID)
            mStaticMeshInstances.back().setParentInstance(getMeshInstance(parentInstance));
    }
    else
    {
        if(!mFreeDynamicMeshIndicies.empty())
        {
            const uint32_t freeIndex = mFreeDynamicMeshIndicies.back();
            mFreeDynamicMeshIndicies.pop_back();

            mInstanceMap[id] = {InstanceType::DynamicMesh, freeIndex};
            mDynamicMeshInstances[freeIndex] = MeshInstance{this, meshID, transformation, materialIndex, materialFlags, name};
        }
        else
        {
            mInstanceMap[id] = {InstanceType::DynamicMesh, mDynamicMeshInstances.size()};
            mDynamicMeshInstances.push_back({this, meshID, transformation, materialIndex, materialFlags, name});
        }

        if(parentInstance != kInvalidInstanceID)
            mDynamicMeshInstances.back().setParentInstance(getMeshInstance(parentInstance));
    }

    return id;
}


void Scene::removeMeshInstance(const InstanceID id)
{
    BELL_ASSERT(mInstanceMap.find(id) != mInstanceMap.end(), "Instance doesn't exist")

    const InstanceInfo& entry = mInstanceMap[id];
    switch(entry.mtype)
    {
        case InstanceType::DynamicMesh:
        {
            mFreeDynamicMeshIndicies.push_back(entry.mIndex);
            mDynamicMeshInstances[entry.mIndex].setInstanceFlags(0);
            break;
        }

        case InstanceType::StaticMesh:
        {
            mFreeStaticMeshIndicies.push_back(entry.mIndex);
            mStaticMeshInstances[entry.mIndex].setInstanceFlags(0);
            break;
        }

        default:
            BELL_TRAP;
    }

    mInstanceMap.erase(id);
}


void Scene::uploadData(Engine* eng)
{
    eng->flushWait();
    eng->clearVertexCache();

    for(const auto& [mesh, meshType] : mSceneMeshes)
    {
        eng->addMeshToBuffer(&mesh);

        if(mesh.hasAnimations())
            eng->addMeshToAnimationBuffer(&mesh);
    }
}


void Scene::computeBounds(const MeshType type)
{
    generateSceneAABB(type == MeshType::Static);

    const float3 sceneSize = mSceneAABB.getSideLengths();
    const float maxSceneDimension = std::max(sceneSize.x, std::max(sceneSize.y, sceneSize.z));
    const uint32_t maxDivisions = std::ceil(std::log2(maxSceneDimension));
    const uint32_t divisions = std::max(3u, maxDivisions > 8u ? maxDivisions - 8u : maxDivisions);

    if(type == MeshType::Static)
    {
        //Build the static meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> staticBVHMeshes{};

        std::transform(mStaticMeshInstances.begin(), mStaticMeshInstances.end(), std::back_inserter(staticBVHMeshes),
                       [](MeshInstance& instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance.getMesh()->getAABB() * instance.getTransMatrix(), &instance}; } );

        OctTreeFactory<MeshInstance*> staticBVHFactory(mSceneAABB, staticBVHMeshes);        

        mStaticMeshBoundingVolume = staticBVHFactory.generateOctTree(divisions);
    }
    else
    {
        //Build the dynamic meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> dynamicBVHMeshes{};

        std::transform(mDynamicMeshInstances.begin(), mDynamicMeshInstances.end(), std::back_inserter(dynamicBVHMeshes),
                       [](MeshInstance& instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance.getMesh()->getAABB() * instance.getTransMatrix(), &instance}; } );


         OctTreeFactory<MeshInstance*> dynamicBVHFactory(mSceneAABB, dynamicBVHMeshes);

         mDynamicMeshBoundingVolume = dynamicBVHFactory.generateOctTree(divisions);
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
    return Frustum(mShadowingLight.mViewProj);
}


MeshInstance* Scene::getMeshInstance(const InstanceID id)
{
    BELL_ASSERT(mInstanceMap.find(id) != mInstanceMap.end(), "Invalid instanceID")

    const InstanceInfo&  entry = mInstanceMap[id];

    switch(entry.mtype)
    {
        case InstanceType::DynamicMesh:
            return &mDynamicMeshInstances[entry.mIndex];

        case InstanceType::StaticMesh:
            return&mStaticMeshInstances[entry.mIndex];

        default:
            BELL_TRAP;
    }

    return nullptr;
}


StaticMesh* Scene::getMesh(const SceneID id)
{
    return &mSceneMeshes[id].first;
}


const StaticMesh* Scene::getMesh(const SceneID id) const
{
    return &mSceneMeshes[id].first;
}


// Find the smallest and largest coords amoung the meshes AABB and set the scene AABB to those.
void Scene::generateSceneAABB(const bool includeStatic)
{
    Cube sceneCube = mSceneAABB.getCube();

    if(includeStatic)
    {
        for(const auto& instance : mStaticMeshInstances)
        {
            AABB meshAABB = instance.getMesh()->getAABB() * instance.getTransMatrix();
            Cube instanceCube = meshAABB.getCube();

            sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

            sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
        }
    }

    for(const auto& instance : mDynamicMeshInstances)
    {
        AABB meshAABB = instance.getMesh()->getAABB() * instance.getTransMatrix();
        Cube instanceCube = meshAABB.getCube();

        sceneCube.mUpper1 = componentWiseMin(instanceCube.mUpper1, sceneCube.mUpper1);

        sceneCube.mLower3 = componentWiseMax(instanceCube.mLower3, sceneCube.mLower3);
    }

    mSceneAABB = AABB(sceneCube);
}


void Scene::setShadowingLight(const Camera& cam)
{
    mShadowLightCamera = cam;

    const float4x4 view = mShadowLightCamera.getViewMatrix();
    const float4x4 proj = mShadowLightCamera.getProjectionMatrix();

    ShadowingLight light{view, glm::inverse(view), proj * view, float4(mShadowLightCamera.getPosition(), 1.0f), float4(mShadowLightCamera.getDirection(), 0.0f), float4(mShadowLightCamera.getUp(), 1.0f)};
    mShadowingLight = light;
}


void Scene::addMaterial(const Scene::Material& mat)
{
    mMaterials.push_back(mat);

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::HeightMap))
    {
        mMaterialImageViews.emplace_back(*mat.mHeightMap, ImageViewType::Colour);
    }

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Albedo) ||
            mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Diffuse))
    {
        mMaterialImageViews.emplace_back(*mat.mAlbedoorDiffuse, ImageViewType::Colour);
    }

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Normals))
    {
        mMaterialImageViews.emplace_back(*mat.mNormals, ImageViewType::Colour);
    }

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::CombinedMetalnessRoughness))
    {
         mMaterialImageViews.emplace_back(*mat.mRoughnessOrGloss, ImageViewType::Colour);
    }

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Roughness) ||
            mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Gloss))
    {
        mMaterialImageViews.emplace_back(*mat.mRoughnessOrGloss, ImageViewType::Colour);
    }

    if((mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Metalness)) ||
            (mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Specular)) ||
            (mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::CombinedSpecularGloss)))
    {
        mMaterialImageViews.emplace_back(*mat.mMetalnessOrSpecular, ImageViewType::Colour);
    }

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::AmbientOcclusion))
    {
        mMaterialImageViews.emplace_back(*mat.mAmbientOcclusion, ImageViewType::Colour);
    }

    if(mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::Emisive))
    {
        mMaterialImageViews.emplace_back(*mat.mEmissive, ImageViewType::Colour);
    }
}


void Scene::addMaterial(const MaterialPaths& mat, Engine* eng)
{
    const uint32_t materialFlags = mat.mMaterialTypes;
    Scene::Material newMaterial{};
    newMaterial.mMaterialTypes = materialFlags;
    newMaterial.mMaterialOffset = mat.mMaterialOffset;

    auto calculateMips = [](const TextureUtil::TextureInfo& info) -> uint32_t
    {
        const int minSize = std::min(info.width, info.height);
        const uint32_t logSize = std::log2(minSize);

        return std::clamp(logSize, 1u, 8u);
    };

    if(materialFlags & static_cast<uint32_t>(MaterialType::HeightMap))
    {
        TextureUtil::TextureInfo heightMapInfo = TextureUtil::load32BitTexture(mat.mHeightMapPath.c_str(), STBI_grey);
        Image* heightTexture = new Image(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(heightMapInfo.width), static_cast<uint32_t>(heightMapInfo.height), 1, calculateMips(heightMapInfo), 1, 1, mat.mHeightMapPath);
        (*heightTexture)->setContents(heightMapInfo.mData.data(), static_cast<uint32_t>(heightMapInfo.width), static_cast<uint32_t>(heightMapInfo.height), 1);
        (*heightTexture)->generateMips();
        newMaterial.mHeightMap = heightTexture;

        mCPUMaterials.emplace_back(std::move(heightMapInfo.mData), (*heightTexture)->getExtent(0, 0), Format::R8UNorm);
    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::Albedo) || materialFlags & static_cast<uint32_t>(MaterialType::Diffuse))
    {
        TextureUtil::TextureInfo diffuseInfo = TextureUtil::load32BitTexture(mat.mAlbedoorDiffusePath.c_str(), STBI_rgb_alpha);
        Image *diffuseTexture = new Image(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1, calculateMips(diffuseInfo), 1, 1, mat.mAlbedoorDiffusePath);
        (*diffuseTexture)->setContents(diffuseInfo.mData.data(), static_cast<uint32_t>(diffuseInfo.width), static_cast<uint32_t>(diffuseInfo.height), 1);
        (*diffuseTexture)->generateMips();

        newMaterial.mAlbedoorDiffuse = diffuseTexture;

        mCPUMaterials.emplace_back(std::move(diffuseInfo.mData), (*diffuseTexture)->getExtent(0, 0), Format::RGBA8UNorm);
    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::Normals))
    {
        TextureUtil::TextureInfo normalsInfo = TextureUtil::load32BitTexture(mat.mNormalsPath.c_str(), STBI_rgb_alpha);
        Image* normalsTexture = new Image(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1, calculateMips(normalsInfo), 1, 1, mat.mNormalsPath);
        (*normalsTexture)->setContents(normalsInfo.mData.data(), static_cast<uint32_t>(normalsInfo.width), static_cast<uint32_t>(normalsInfo.height), 1);
        (*normalsTexture)->generateMips();
        newMaterial.mNormals = normalsTexture;

        mCPUMaterials.emplace_back(std::move(normalsInfo.mData), (*normalsTexture)->getExtent(0, 0), Format::RGBA8UNorm);
    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::Roughness) || materialFlags & static_cast<uint32_t>(MaterialType::Gloss))
    {
        TextureUtil::TextureInfo roughnessInfo = TextureUtil::load32BitTexture(mat.mRoughnessOrGlossPath.c_str(), STBI_grey);
        Image* roughnessTexture = new Image(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1, calculateMips(roughnessInfo), 1, 1, mat.mRoughnessOrGlossPath);
        (*roughnessTexture)->setContents(roughnessInfo.mData.data(), static_cast<uint32_t>(roughnessInfo.width), static_cast<uint32_t>(roughnessInfo.height), 1);
        (*roughnessTexture)->generateMips();
        newMaterial.mRoughnessOrGloss = roughnessTexture;

        mCPUMaterials.emplace_back(std::move(roughnessInfo.mData), (*roughnessTexture)->getExtent(0, 0), Format::R8UNorm);
    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::Metalness) || materialFlags & static_cast<uint32_t>(MaterialType::Specular) || mat.mMaterialTypes & static_cast<uint32_t>(MaterialType::CombinedSpecularGloss))
    {
        TextureUtil::TextureInfo metalnessInfo = TextureUtil::load32BitTexture(mat.mMetalnessOrSpecularPath.c_str(), materialFlags & static_cast<uint32_t>(MaterialType::Metalness) ? STBI_grey : STBI_rgb_alpha);
        Image* metalnessTexture = new Image(eng->getDevice(), materialFlags & static_cast<uint32_t>(MaterialType::Metalness) ? Format::R8UNorm : Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(metalnessInfo.width), static_cast<uint32_t>(metalnessInfo.height), 1, calculateMips(metalnessInfo), 1, 1, mat.mMetalnessOrSpecularPath);
        (*metalnessTexture)->setContents(metalnessInfo.mData.data(), static_cast<uint32_t>(metalnessInfo.width), static_cast<uint32_t>(metalnessInfo.height), 1);
        (*metalnessTexture)->generateMips();
        newMaterial.mMetalnessOrSpecular = metalnessTexture;

        mCPUMaterials.emplace_back(std::move(metalnessInfo.mData), (*metalnessTexture)->getExtent(0, 0), materialFlags & static_cast<uint32_t>(MaterialType::Metalness) ? Format::R8UNorm : Format::RGBA8UNorm);
    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::CombinedMetalnessRoughness))
    {
        TextureUtil::TextureInfo metalnessRoughnessInfo = TextureUtil::load32BitTexture(mat.mRoughnessOrGlossPath.c_str(), STBI_rgb_alpha);
        Image* metalnessRoughnessTexture = new Image(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(metalnessRoughnessInfo.width), static_cast<uint32_t>(metalnessRoughnessInfo.height), 1, calculateMips(metalnessRoughnessInfo), 1, 1, mat.mRoughnessOrGlossPath);
        (*metalnessRoughnessTexture)->setContents(metalnessRoughnessInfo.mData.data(), static_cast<uint32_t>(metalnessRoughnessInfo.width), static_cast<uint32_t>(metalnessRoughnessInfo.height), 1);
        (*metalnessRoughnessTexture)->generateMips();
        newMaterial.mRoughnessOrGloss = metalnessRoughnessTexture;

        mCPUMaterials.emplace_back(std::move(metalnessRoughnessInfo.mData), (*metalnessRoughnessTexture)->getExtent(0, 0), Format::RGBA8UNorm);

    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::Emisive))
    {
        TextureUtil::TextureInfo emissiveInfo = TextureUtil::load32BitTexture(mat.mEmissivePath.c_str(), STBI_rgb_alpha);
        Image* emissiveTexture = new Image(eng->getDevice(), Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(emissiveInfo.width), static_cast<uint32_t>(emissiveInfo.height), 1, calculateMips(emissiveInfo), 1, 1, mat.mEmissivePath);
        (*emissiveTexture)->setContents(emissiveInfo.mData.data(), static_cast<uint32_t>(emissiveInfo.width), static_cast<uint32_t>(emissiveInfo.height), 1);
        (*emissiveTexture)->generateMips();
        newMaterial.mEmissive = emissiveTexture;

        mCPUMaterials.emplace_back(std::move(emissiveInfo.mData), (*emissiveTexture)->getExtent(0, 0), Format::RGBA8UNorm);
    }

    if(materialFlags & static_cast<uint32_t>(MaterialType::AmbientOcclusion))
    {
        TextureUtil::TextureInfo occlusionInfo = TextureUtil::load32BitTexture(mat.mAmbientOcclusionPath.c_str(), STBI_grey);
        Image* occlusionTexture = new Image(eng->getDevice(), Format::R8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest | ImageUsage::TransferSrc,
                             static_cast<uint32_t>(occlusionInfo.width), static_cast<uint32_t>(occlusionInfo.height), 1, calculateMips(occlusionInfo), 1, 1, mat.mAmbientOcclusionPath);
        (*occlusionTexture)->setContents(occlusionInfo.mData.data(), static_cast<uint32_t>(occlusionInfo.width), static_cast<uint32_t>(occlusionInfo.height), 1);
        (*occlusionTexture)->generateMips();
        newMaterial.mAmbientOcclusion = occlusionTexture;

        mCPUMaterials.emplace_back(std::move(occlusionInfo.mData), (*occlusionTexture)->getExtent(0, 0), Format::R8UNorm);
    }

    newMaterial.mName = mat.mName;
    addMaterial(newMaterial);
}


std::vector<Scene::Intersection> Scene::getIntersections() const
{
	std::vector<Intersection> intersections;

	for (auto& mesh : mStaticMeshInstances)
	{
        const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(mesh.getMesh()->getAABB() * mesh.getTransMatrix());
        const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(mesh.getMesh()->getAABB() * mesh.getTransMatrix());

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
        const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(mesh.getMesh()->getAABB() * mesh.getTransMatrix());
        const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(mesh.getMesh()->getAABB() * mesh.getTransMatrix());

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

    const std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.getIntersections(instanceToTest->getMesh()->getAABB() * instanceToTest->getTransMatrix());
    const std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.getIntersections(instanceToTest->getMesh()->getAABB() * instanceToTest->getTransMatrix());

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
