#include "Engine/Scene.h"
#include "Engine/Engine.hpp"
#include "Engine/TextureUtil.hpp"
#include "Engine/UberShaderStateCache.hpp"
#include "Core/BellLogging.hpp"
#include "Core/Profiling.hpp"
#include "Core/Executor.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <limits>

namespace
{
    aiMatrix4x4 glmToAi(const float4x4& transformation)
    {
        aiMatrix4x4 transformationMatrix{};
        transformationMatrix.a1 = transformation[0][0]; transformationMatrix.b1 = transformation[0][1];  transformationMatrix.c1 = transformation[0][2]; transformationMatrix.d1 = transformation[0][3];
        transformationMatrix.a2 = transformation[1][0]; transformationMatrix.b2 = transformation[1][1];  transformationMatrix.c2 = transformation[1][2]; transformationMatrix.d2 = transformation[1][3];
        transformationMatrix.a3 = transformation[2][0]; transformationMatrix.b3 = transformation[2][1];  transformationMatrix.c3 = transformation[2][2]; transformationMatrix.d3 = transformation[2][3];
        transformationMatrix.a4 = transformation[3][0]; transformationMatrix.b4 = transformation[3][1];  transformationMatrix.c4 = transformation[3][2]; transformationMatrix.d4 = transformation[3][3];

        return transformationMatrix;
    }
}

MeshInstance::MeshInstance(Scene* scene,
                            SceneID meshID,
                            InstanceID id,
                            const float4x3& trans,
                            const uint32_t materialID,
                            const uint32_t materialFLags,
                            const std::string& name)  :
        Instance(trans, name),
        mScene(scene),
        mMesh(meshID),
        mID{id},
        mMaterials{},
        mInstanceFlags{InstanceFlags::Draw},
        mGlobalBoneBufferOffset(0),
        mIsSkinned(false),
        mAnimationActive(false)
{
    const StaticMesh* mesh = scene->getMesh(meshID);
    const std::vector<SubMesh>& submeshes = mesh->getSubMeshes();
    mMaterials.resize(submeshes.size());
    for(auto&[materialIndex, materialFlags] : mMaterials)
    {
        materialIndex = materialID;
        materialFlags = materialFLags;
    }

    mIsSkinned = mesh->hasAnimations();
}

MeshInstance::MeshInstance(Scene* scene,
                            SceneID meshID,
                           InstanceID id,
                            const float3& position,
                            const quat& rotation,
                            const float3& scale,
                            const uint32_t materialID,
                            const uint32_t materialFLags,
                            const std::string& name) :
        Instance(position, rotation, scale, name),
        mScene(scene),
        mMesh(meshID),
        mID{id},
        mMaterials{},
        mInstanceFlags{InstanceFlags::Draw},
        mGlobalBoneBufferOffset(0),
        mIsSkinned(false),
        mAnimationActive(false)
{
    const StaticMesh* mesh = scene->getMesh(meshID);
    const std::vector<SubMesh>& submeshes = mesh->getSubMeshes();
    mMaterials.resize(submeshes.size());
    for(auto&[materialIndex, materialFlags] : mMaterials)
    {
        materialIndex = materialID;
        materialFlags = materialFLags;
    }

    mIsSkinned = mesh->hasAnimations();
}


StaticMesh* MeshInstance::getMesh()
{
    return mScene->getMesh(mMesh);
}

const StaticMesh* MeshInstance::getMesh() const
{
    return mScene->getMesh(mMesh);
}

BottomLevelAccelerationStructure* MeshInstance::getAccelerationStructure()
{
    return mScene->getAccelerationStructure(mMesh);
}

const BottomLevelAccelerationStructure* MeshInstance::getAccelerationStructure() const
{
    return mScene->getAccelerationStructure(mMesh);
}

std::vector<float4x4> MeshInstance::tickAnimation(const double time)
{
    const SkeletalAnimation* activeAnim = getActiveAnimation();
    if(!activeAnim || !mAnimationActive)
        return std::vector<float4x4>(getMesh()->getSkeleton().size(), float4x4(1.0f));

    mTick += time * activeAnim->getTicksPerSec();

    if(mTick >= activeAnim->getTotalTicks() && !mLoop)
    {
        mAnimationActive = false;
        return std::vector<float4x4>(getMesh()->getSkeleton().size(), float4x4(1.0f));
    }

    if(mLoop)
        mTick = fmod(mTick, activeAnim->getTotalTicks());

    return activeAnim->calculateBoneMatracies(*getMesh(), mTick);
}

void MeshInstance::draw(Executor* exec, UberShaderStateCache* cache) const
{
    const StaticMesh* mesh = getMesh();
    const uint32_t vertesStride = mesh->getVertexStride();
    const std::vector<SubMesh>& subMeshes = mesh->getSubMeshes();

    exec->bindVertexBuffer(*(mesh->getVertexBufferView()), 0);
    exec->bindIndexBuffer(*(mesh->getIndexBufferView()), 0);

    for(uint32_t subMesh_i = 0; subMesh_i < subMeshes.size(); ++subMesh_i)
    {
        const SubMesh& subMesh = subMeshes[subMesh_i];
        MeshEntry shaderEntry = getMeshShaderEntry(subMesh_i);
        const uint64_t shadeFlags = getShadeFlags(subMesh_i);
        cache->update(shadeFlags);

        exec->insertPushConstant(&shaderEntry, sizeof(MeshEntry));
        exec->indexedDraw(subMesh.mVertexOffset, subMesh.mIndexOffset, subMesh.mIndexCount);
    }
}

uint64_t MeshInstance::getShadeFlags(const uint32_t subMesh_i) const
{
    BELL_ASSERT(subMesh_i < mMaterials.size(), "submesh out of index")
    return mMaterials[subMesh_i].mMaterialFlags | (mScene->getMesh(mMesh)->getSkeleton().empty() ? 0 : kShade_Skinning);
}

const SkeletalAnimation* MeshInstance::getActiveAnimation() const
{
    if(!mAnimationActive && mActiveAnimationName == "")
        return nullptr;

    const StaticMesh* mesh = getMesh();
    const SkeletalAnimation& anim = mesh->getSkeletalAnimation(mActiveAnimationName);

    return &anim;
}


Scene::Scene(const std::filesystem::path& path) :
    mPath{path},
    mSceneMeshes(),
    mStaticMeshBoundingVolume(),
    mDynamicMeshBoundingVolume(),
    mLightsBoundingVolume(),
    mRootTransform{1.0f},
    mSceneAABB(float4(std::numeric_limits<float>::max()), float4(std::numeric_limits<float>::min())),
    mSceneCamera(nullptr),
	mMaterials{},
	mMaterialImageViews{},
    mLights{},
    mFreeLightIndicies{},
    mShadowLightCamera(nullptr),
    mShadowingLight{},
    mCascadesInfo{0.1f, 0.4f, 1.0f},
    mNextInstanceID{0},
	mSkybox{nullptr}
{
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


void Scene::loadSkybox(const std::array<std::string, 6>& paths, RenderEngine* eng)
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


std::vector<InstanceID> Scene::loadFromFile(const int vertAttributes, RenderEngine* eng)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(mPath.string().c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_GenNormals |
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_GlobalScale |
                                             aiProcess_FlipUVs);

    const aiNode* rootNode = scene->mRootNode;

    mSceneMeshes.reserve(scene->mNumMeshes);
    mSceneAccelerationStructures.reserve(scene->mNumMeshes);

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
    parseNode(eng,
              scene,
              rootNode,
              glmToAi(mRootTransform),
              kInvalidInstanceID,
              vertAttributes,
              meshMaterials,
              meshToSceneIDMapping,
              instanceIDs);

    addLights(scene);

    return instanceIDs;
}


void Scene::parseNode(RenderEngine* eng,
                      const aiScene* scene,
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

            meshID = addMesh(eng, mesh, MeshType::Static);

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
        parseNode(eng,
                  scene,
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
        sceneLight.mAngleSize = float3{45.0f, 0.0f, 0.0f};

        mLights.push_back(sceneLight);
    }
}


void Scene::loadMaterials(RenderEngine* eng)
{
    const MaterialMappings mappings = loadMaterialsInternal(eng);
}


Scene::MaterialMappings Scene::loadMaterialsInternal(RenderEngine* eng)
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


void Scene::loadMaterialsExternal(RenderEngine* eng, const aiScene* scene)
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


SceneID Scene::addMesh(RenderEngine* eng, const StaticMesh& mesh, MeshType meshType)
{
    SceneID id = mSceneMeshes.size();
    mSceneMeshes.emplace_back(mesh, meshType);

    if(eng->getDevice()->getDeviceFeatureFlags() & DeviceFeaturesFlags::RayTracing)
        mSceneAccelerationStructures.emplace_back(eng, mesh);

    return id;
}


SceneID Scene::loadFile(const std::string &path, MeshType meshType, RenderEngine* eng, const bool loadMaterials)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path.c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_GenNormals |
                                             aiProcess_CalcTangentSpace |
                                             aiProcess_GlobalScale |
                                             aiProcess_FlipUVs);

    StaticMesh newMesh(scene, VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates |
                       VertexAttributes::Tangents | VertexAttributes::Albedo);
    newMesh.initializeDeviceBuffers(eng);
    const InstanceID id = addMesh(eng, newMesh, meshType);

    mPath = fs::path(path);

    if(loadMaterials)
    {
        fs::path materialFile{mPath};
        materialFile += ".mat";
        if (fs::exists(materialFile))
            loadMaterialsInternal(eng); // Load materials from the internal .mat format.
        else
            loadMaterialsExternal(eng, scene);
    }

    return id;
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

    const InstanceID id = mNextInstanceID++;

    if(meshType == MeshType::Static)
    {
        if(!mFreeStaticMeshIndicies.empty())
        {
            const uint32_t freeIndex = mFreeStaticMeshIndicies.back();
            mFreeStaticMeshIndicies.pop_back();

            mInstanceMap[id] = {InstanceType::StaticMesh, freeIndex};
            mStaticMeshInstances[freeIndex] = MeshInstance{this, meshID, id, transformation, materialIndex, materialFlags, name};
        }
        else
        {
            mInstanceMap[id] = {InstanceType::StaticMesh, mStaticMeshInstances.size()};
            mStaticMeshInstances.push_back({this, meshID, id, transformation, materialIndex, materialFlags, name});
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
            mDynamicMeshInstances[freeIndex] = MeshInstance{this, meshID, id, transformation, materialIndex, materialFlags, name};
        }
        else
        {
            mInstanceMap[id] = {InstanceType::DynamicMesh, mDynamicMeshInstances.size()};
            mDynamicMeshInstances.push_back({this, meshID, id, transformation, materialIndex, materialFlags, name});
        }

        if(parentInstance != kInvalidInstanceID)
            mDynamicMeshInstances.back().setParentInstance(getMeshInstance(parentInstance));
    }

    return id;
}


InstanceID    Scene::addMeshInstance( const SceneID meshID,
                                      const InstanceID parentInstance,
                                      const float3& position,
                                      const float3& scale,
                                      const quat& rotation,
                                      const uint32_t materialIndex,
                                      const uint32_t materialFlags,
                                      const std::string& name)
{
    auto& [mesh, meshType] = mSceneMeshes[meshID];

    const float4x4 requiredTransform = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);

    return addMeshInstance(meshID, parentInstance, requiredTransform, materialIndex, materialFlags, name);
}



void Scene::removeInstance(const InstanceID id)
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

        case InstanceType::Light:
        {
            mFreeLightIndicies.push_back(entry.mIndex);
            mLights[entry.mIndex].mIntensity = 0.0f;
            break;
        }

        default:
            BELL_TRAP;
    }

    mInstanceMap.erase(id);
}


void Scene::computeBounds(const AccelerationStructure type)
{
    PROFILER_EVENT();

    generateSceneAABB(type == AccelerationStructure::StaticMesh);

    if(type == AccelerationStructure::StaticMesh)
    {
        //Build the static meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> staticBVHMeshes{};
        std::vector<MeshInstance*> activeInstances{};
        activeInstances.reserve(mStaticMeshInstances.size());
        for(auto& instance : mStaticMeshInstances)
        {
            if(instance.getInstanceFlags() != 0)
                activeInstances.push_back(&instance);
        }

        std::transform(activeInstances.begin(), activeInstances.end(), std::back_inserter(staticBVHMeshes),
                       [](MeshInstance* instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance->getMesh()->getAABB() * instance->getTransMatrix(), instance}; } );

        OctTreeFactory<MeshInstance*> staticBVHFactory(mSceneAABB, staticBVHMeshes);        

        mStaticMeshBoundingVolume = staticBVHFactory.generateOctTree();
    }
    else if(type == AccelerationStructure::DynamicMesh)
    {
        //Build the dynamic meshes BVH structure.
        std::vector<typename OctTreeFactory<MeshInstance*>::BuilderNode> dynamicBVHMeshes{};
        std::vector<MeshInstance*> activeInstances{};
        activeInstances.reserve(mDynamicMeshInstances.size());
        for(auto& instance : mDynamicMeshInstances)
        {
            if(instance.getInstanceFlags() != 0)
                activeInstances.push_back(&instance);
        }

        std::transform(activeInstances.begin(), activeInstances.end(), std::back_inserter(dynamicBVHMeshes),
                       [](MeshInstance* instance)
                        { return OctTreeFactory<MeshInstance*>::BuilderNode{instance->getMesh()->getAABB() * instance->getTransMatrix(), instance}; } );


         OctTreeFactory<MeshInstance*> dynamicBVHFactory(mSceneAABB, dynamicBVHMeshes);

         mDynamicMeshBoundingVolume = dynamicBVHFactory.generateOctTree();
    }
    else if(type == AccelerationStructure::Lights)
    {
        std::vector<typename OctTreeFactory<Light*>::BuilderNode> lightBVHMeshes{};
        std::vector<Light*> activeInstances{};
        activeInstances.reserve(mLights.size());
        for(auto& light : mLights)
        {
            if(light.mIntensity != 0.0f)
                activeInstances.push_back(&light);
        }

        std::transform(activeInstances.begin(), activeInstances.end(), std::back_inserter(lightBVHMeshes),
                       [](Light* instance)
                       { return OctTreeFactory<Light*>::BuilderNode{instance->getAABB(), instance}; } );


        OctTreeFactory<Light*> lightBVHFactory(mSceneAABB, lightBVHMeshes);

        mLightsBoundingVolume = lightBVHFactory.generateOctTree();
    }
}


std::vector<MeshInstance*> Scene::getVisibleMeshes(const Frustum& frustum) const
{
    PROFILER_EVENT();

    std::vector<MeshInstance*> instances;

    std::vector<MeshInstance*> staticMeshes = mStaticMeshBoundingVolume.containedWithin(frustum);
    std::vector<MeshInstance*> dynamicMeshes = mDynamicMeshBoundingVolume.containedWithin(frustum);

    instances.insert(instances.end(), staticMeshes.begin(), staticMeshes.end());
    instances.insert(instances.end(), dynamicMeshes.begin(), dynamicMeshes.end());

    return instances;
}


std::vector<Scene::Light*> Scene::getVisibleLights(const Frustum& frustum) const
{
    return mLightsBoundingVolume.containedWithin(frustum);
}


Frustum Scene::getShadowingLightFrustum() const
{
    return Frustum(mShadowingLight.mViewProj);
}


MeshInstance* Scene::getMeshInstance(const InstanceID id)
{
    BELL_ASSERT(mInstanceMap.find(id) != mInstanceMap.end(), "Invalid instanceID")
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

const MeshInstance* Scene::getMeshInstance(const InstanceID id) const
{
    BELL_ASSERT(mInstanceMap.find(id) != mInstanceMap.end(), "Invalid instanceID")

    const InstanceInfo& entry = (*mInstanceMap.find(id)).second;

    switch (entry.mtype)
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

float3 Scene::getInstancePosition(const InstanceID id) const
{
    const MeshInstance* inst = getMeshInstance(id);

    return inst->getPosition();
}

void   Scene::setInstancePosition(const InstanceID id, const float3& pos)
{
    MeshInstance* inst = getMeshInstance(id);
    inst->setPosition(pos);
}


void   Scene::translateInstance(const InstanceID id, const float3& v)
{
    float3 pos = getInstancePosition(id);
    pos += v;
    setInstancePosition(id, pos);
}


StaticMesh* Scene::getMesh(const SceneID id)
{
    return &mSceneMeshes[id].first;
}


const StaticMesh* Scene::getMesh(const SceneID id) const
{
    return &mSceneMeshes[id].first;
}


BottomLevelAccelerationStructure* Scene::getAccelerationStructure(const SceneID id)
{
    if(!mSceneAccelerationStructures.empty())
        return &mSceneAccelerationStructures[id];
    else
        return nullptr;
}


const BottomLevelAccelerationStructure* Scene::getAccelerationStructure(const SceneID id) const
{
    if(!mSceneAccelerationStructures.empty())
        return &mSceneAccelerationStructures[id];
    else
        return nullptr;
}


void Scene::initializeDeviceBuffers(RenderEngine* eng)
{
    for(auto& [mesh, type] : mSceneMeshes)
    {
        if(!mesh.getVertexBuffer() || !mesh.getIndexBuffer())
            mesh.initializeDeviceBuffers(eng);
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


void Scene::updateShadowingLight()
{
    if(!mShadowLightCamera)
        return;

    const float4x4 view = mShadowLightCamera->getViewMatrix();
    const float4x4 proj = mShadowLightCamera->getProjectionMatrix();

    ShadowingLight light{view, glm::inverse(view), proj * view, float4(mShadowLightCamera->getPosition(), 1.0f), float4(mShadowLightCamera->getDirection(), 0.0f), float4(mShadowLightCamera->getUp(), 1.0f)};
    mShadowingLight = light;
}


void Scene::setShadowingLight(Camera* cam)
{
    BELL_ASSERT(cam, "Camera can't be null")
    mShadowLightCamera = cam;

    updateShadowingLight();
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


void Scene::addMaterial(const MaterialPaths& mat, RenderEngine* eng)
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


InstanceID Scene::addLight(const Light& light)
{
    const InstanceID id = mNextInstanceID++;

    if(!mFreeLightIndicies.empty())
    {
        const uint32_t freeIndex = mFreeLightIndicies.back();
        mFreeLightIndicies.pop_back();

        mInstanceMap[id] = {InstanceType::Light, freeIndex};
        mLights[freeIndex] = light;
    }
    else
    {
        mInstanceMap[id] = {InstanceType::Light, mLights.size()};
        mLights.push_back(light);
    }

    return id;
}


Scene::Light& Scene::getLight(const InstanceID id)
{
    BELL_ASSERT(mInstanceMap.find(id) != mInstanceMap.end(), "Invalid ID")
    BELL_ASSERT(mInstanceMap.find(id)->second.mtype == InstanceType::Light, "Invalid ID")

    const uint32_t index = mInstanceMap[id].mIndex;
    BELL_ASSERT(index < mLights.size(), "invalid light index")

    return mLights[index];
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
    light.mAngleSize = float3{angle, 0.0f, 0.0f};

    return light;
}


Scene::Light Scene::Light::areaLight(const float4& position, const float4& direction, const float4& up, const float4& albedo, const float intensity, const float radius, const float2 size)
{
    Scene::Light light{};
    light.mPosition = position;
    light.mDirection = direction;
    light.mUp = up;
    light.mAlbedo = albedo;
    light.mIntensity = intensity;
    light.mType = LightType::Area;
    light.mRadius = radius;
    light.mAngleSize = float3{size, 0.0f};


    return light;
}


Scene::Light Scene::Light::stripLight(const float4& position, const float4& direction, const float4& albedo, const float intensity, const float radius, const float2 size)
{
    Scene::Light light{};
    light.mPosition = position;
    light.mDirection = direction;
    light.mAlbedo = albedo;
    light.mIntensity = intensity;
    light.mType = LightType::Strip;
    light.mRadius = radius;
    light.mAngleSize = float3{size, 0.0f};

    return light;
}


AABB Scene::Light::getAABB() const
{
    const float maxDiameter = sqrt(mRadius * mRadius * 2.0f);
    const float4 boundsMin = float4(mPosition - (0.5f * maxDiameter), 1.0f);
    const float4 boundsMax = float4(mPosition + (0.5f * maxDiameter), 1.0f);

    return AABB{boundsMin, boundsMax};
}
