#include "Engine/StaticMesh.h"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include "glm/gtx/handed_coordinate_space.hpp"

#include <limits>


namespace
{
    float4x4 aiMatrix4x4ToFloat4x4(const aiMatrix4x4& transformation)
    {
        float4x4 transformationMatrix{};
        transformationMatrix[0][0] = transformation.a1; transformationMatrix[0][1] = transformation.b1;  transformationMatrix[0][2] = transformation.c1; transformationMatrix[0][3] = transformation.d1;
        transformationMatrix[1][0] = transformation.a2; transformationMatrix[1][1] = transformation.b2;  transformationMatrix[1][2] = transformation.c2; transformationMatrix[1][3] = transformation.d2;
        transformationMatrix[2][0] = transformation.a3; transformationMatrix[2][1] = transformation.b3;  transformationMatrix[2][2] = transformation.c3; transformationMatrix[2][3] = transformation.d3;
        transformationMatrix[3][0] = transformation.a4; transformationMatrix[3][1] = transformation.b4;  transformationMatrix[3][2] = transformation.c4; transformationMatrix[3][3] = transformation.d4;

        return transformationMatrix;
    }
}


StaticMesh::StaticMesh(const std::string& path, const int vertAttributes, const bool globalScaling) :
	mVertexData{},
	mIndexData{},
    mAABB{{INFINITY, INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY, -INFINITY}},
    mBoneCount(0),
	mVertexCount(0),
    mVertexAttributes(vertAttributes),
    mVertexStride(0)
{
	Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path.c_str(),
											 aiProcess_Triangulate |
											 aiProcess_JoinIdenticalVertices |
											 aiProcess_GenNormals |
                                             aiProcess_CalcTangentSpace |
                                             (globalScaling ? aiProcess_GlobalScale : 0) |
											 aiProcess_FlipUVs);

    parseNode(scene,
              scene->mRootNode,
              aiMatrix4x4(),
              mVertexAttributes);

    loadAnimations(scene);
}


StaticMesh::StaticMesh(const aiScene *scene, const aiMesh* mesh, const int vertexAttributes) :
    mAABB{{INFINITY, INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY, -INFINITY}},
    mVertexAttributes(vertexAttributes),
    mBoneCount(0),
    mVertexCount(0)
{
    configure(scene, mesh, float4x4(1.0f), vertexAttributes);

    loadAnimations(scene);
}


StaticMesh::StaticMesh(const aiScene* scene, const int vertexAttributes) :
        mAABB{{INFINITY, INFINITY, INFINITY, INFINITY}, {-INFINITY, -INFINITY, -INFINITY, -INFINITY}},
        mVertexAttributes(vertexAttributes),
        mBoneCount(0),
        mVertexCount(0)
{
    parseNode(scene,
              scene->mRootNode,
              aiMatrix4x4(),
              vertexAttributes);

    loadAnimations(scene);
}


void StaticMesh::configure(const aiScene* scene, const aiMesh* mesh, const float4x4 transform, const int vertAttributes)
{
    const unsigned int primitiveType = mesh->mPrimitiveTypes;

    const uint32_t primitiveSize = getPrimitiveSize(static_cast<aiPrimitiveType>(primitiveType));

    const bool positionNeeded = mesh->HasPositions() && ((vertAttributes & VertexAttributes::Position2 ||
                                                     vertAttributes & VertexAttributes::Position3 ||
                                                     vertAttributes & VertexAttributes::Position4));

    const bool UVNeeded = mesh->HasTextureCoords(0) && (vertAttributes & VertexAttributes::TextureCoordinates);

    const bool normalsNeeded = mesh->HasNormals() && (vertAttributes & VertexAttributes::Normals);

    const bool tangentsNeeded = mesh->HasTangentsAndBitangents() && (vertAttributes & VertexAttributes::Tangents);

    const bool albedoNeeded = (mesh->GetNumColorChannels() > 0) && (vertAttributes & VertexAttributes::Albedo);

    // relys on float and MaterialID beingn the same size (should always be true).
    static_assert(sizeof(float) == sizeof(uint32_t), "Material ID doesn't match sizeof(float");
    const uint32_t vertexStride =   ((positionNeeded ? primitiveSize : 0) +
                                    ((vertAttributes & VertexAttributes::TextureCoordinates) ? 2 : 0) +
                                    (normalsNeeded ? 1 : 0) +
                                    (vertAttributes & VertexAttributes::Tangents ? 1 : 0) +
                                    ((vertAttributes & VertexAttributes::Albedo) ? 1 : 0)) * sizeof(float);

	mVertexStride = vertexStride;
	mVertexCount += mesh->mNumVertices;

	SubMesh newSubMesh{};
	newSubMesh.mIndexOffset = mIndexData.size();
	newSubMesh.mIndexCount = mesh->mNumFaces * mesh->mFaces[0].mNumIndices;
	newSubMesh.mVertexOffset = mVertexData.getVertexBuffer().size() / mVertexStride;
	newSubMesh.mTransform = transform;

    // assume triangles atm
    mIndexData.reserve(mIndexData.size() + (mesh->mNumFaces * mesh->mFaces[0].mNumIndices));
    mVertexData.setSize(mVertexData.getVertexBuffer().size() + (mesh->mNumVertices * vertexStride));

    // Copy the index data
    for(uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        BELL_ASSERT(mesh->mFaces[i].mNumIndices == 3, "Isn't a triangle list")

        for(uint32_t j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
        {
            BELL_ASSERT(mesh->mFaces[i].mIndices[j] < mVertexCount, "Index out of bounds")
            mIndexData.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }

    float4 topLeft{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 1.0f};
    float4 bottumRight{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), 1.0f};

    // Copy the vertex buffer data.
    for(uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        if(positionNeeded)
        {
            mVertexData.writeVertexVector4(mesh->mVertices[i]);

            // update the AABB positions
            topLeft = componentWiseMin(topLeft, float4{	mesh->mVertices[i].x,
                                                        mesh->mVertices[i].y,
                                                        mesh->mVertices[i].z, 1.0f});

            bottumRight = componentWiseMax(bottumRight, float4{	mesh->mVertices[i].x,
                                                                mesh->mVertices[i].y,
                                                                mesh->mVertices[i].z, 1.0f});
        }

        if(UVNeeded)
        {
            mVertexData.writeVertexVector2({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y});
        }
        else if(vertAttributes & VertexAttributes::TextureCoordinates) // If the file doesn't contain uv's (and they where requested) write out some placeholder ones.
        {
            mVertexData.writeVertexVector2(aiVector2D{0.0f, 0.0f});
            BELL_LOG("Inserting placeholder UVs")
        }

        if(normalsNeeded)
        {
            static_assert(sizeof(char4) == sizeof(uint32_t), "char4 must match 32 bit int in size");
            float4 normal = float4(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 1.0f);
            const char4 packednormals = packNormal(normal);
            mVertexData.WriteVertexChar4(packednormals);
        }

        if (tangentsNeeded)
        {
            //Need to check if inversion is needed for calculated bi-tangent in shader.
            float3 normal = float3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            float3 tangent = float3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            float3 biTangent = float3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);

            float3 calculatedBitangent = glm::normalize(glm::cross(normal, tangent));
            float bitangentSign;
            if (glm::leftHanded(tangent, biTangent, normal) == glm::leftHanded(tangent, calculatedBitangent, normal))
                bitangentSign = 1.0f;
            else
                bitangentSign = -1.0f;

            static_assert(sizeof(char4) == sizeof(uint32_t), "char4 must match 32 bit int in size");
            const char4 packedtangent = packNormal(float4(tangent, bitangentSign));
            mVertexData.WriteVertexChar4(packedtangent);
        }
        else if(vertAttributes & VertexAttributes::Tangents) // Assume no normal mapping so just fill with a 0.
        {
            mVertexData.WriteVertexChar4({0, 0, 0, 0});
        }

        if(albedoNeeded)
        {
            uint32_t packedColour = uint32_t(mesh->mColors[0][i].r * 255.0f) | (uint32_t(mesh->mColors[0][i].g * 255.0f) << 8) | (uint32_t(mesh->mColors[0][i].b * 255.0f) << 16) | (uint32_t(mesh->mColors[0][i].a * 255.0f) << 24);
            mVertexData.WriteVertexInt(packedColour);
        }
        else if(vertAttributes & VertexAttributes::Albedo) // Default to {0.65f, 0.65f, 0.65f} if no material present.
        {
            uint32_t packedDiffuse = packColour(float4(0.65f, 0.65f, 0.65f, 1.0f));

            const uint32_t matIndex = mesh->mMaterialIndex;
            const aiMaterial* mat = scene->mMaterials[matIndex];
            aiColor4D diffuse;
            const aiReturn result = mat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
            if(result == aiReturn_SUCCESS)
            {
                packedDiffuse = packColour(float4(diffuse.r, diffuse.g, diffuse.b, diffuse.a));
            }

            mVertexData.WriteVertexInt(packedDiffuse);
        }
    }

    AABB submeshAABB{topLeft, bottumRight};
    submeshAABB = submeshAABB * transform;
    mAABB = AABB{componentWiseMin(submeshAABB.getMin(), mAABB.getMin()), componentWiseMax(submeshAABB.getMax(), mAABB.getMax())};

    loadSkeleton(scene, mesh, newSubMesh);
    loadBlendMeshed(mesh);

    mSubMeshes.push_back(newSubMesh);
}


uint16_t StaticMesh::findBoneParent(const aiNode* bone, aiBone** const skeleton, const uint32_t boneCount)
{
    const aiNode* currentNode = bone;
    while(currentNode->mParent)
    {
        const aiNode* parent = currentNode->mParent;
        for(uint16_t i = 0; i < boneCount; ++i)
        {
            if(parent->mName == skeleton[i]->mName)
                return i;
        }

        currentNode = parent;
    }

    return 0xFFFF;
}


void StaticMesh::loadSkeleton(const aiScene* scene, const aiMesh* mesh, SubMesh& submesh)
{
    if(mesh->mNumBones > 0)
    {
        submesh.mSkeleton.reserve(mesh->mNumBones);
        std::vector<BoneIndicies> bonesPerVertex;
        bonesPerVertex.resize(mVertexCount);
        submesh.mBoneWeightsIndicies.resize(mVertexCount);

        const aiNode* rootNode = scene->mRootNode;

        for(uint32_t i = 0; i < mesh->mNumBones; ++i)
        {
            const aiBone* assimpBone = mesh->mBones[i];

            Bone bone{};
            bone.mName = assimpBone->mName.C_Str();
            bone.mInverseBindPose = aiMatrix4x4ToFloat4x4(assimpBone->mOffsetMatrix);

            const aiNode* boneNode = rootNode->FindNode(assimpBone->mName);
            BELL_ASSERT(boneNode, "Can't find bone node")
            bone.mParentIndex = findBoneParent(boneNode, mesh->mBones, mesh->mNumBones);

            float4 topLeft{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), 1.0f};
            float4 bottumRight{-std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(), 1.0f};

            for(uint32_t j = 0; j < assimpBone->mNumWeights; ++j)
            {   
                const aiVertexWeight& weight = assimpBone->mWeights[j];
                BELL_ASSERT(weight.mVertexId < bonesPerVertex.size(), "Invalid vertex index")

                // update AABB
                if(weight.mWeight >= 0.5f)
                {
                    aiVector3D vertex = mesh->mVertices[weight.mVertexId];
                    topLeft = componentWiseMin(topLeft, float4{vertex.x, vertex.y, vertex.z, 1.0f});
                    bottumRight = componentWiseMax(bottumRight, float4{vertex.x, vertex.y, vertex.z, 1.0f});
                }

                BoneIndicies& indicies = bonesPerVertex[weight.mVertexId];
                indicies.mBoneIndices.emplace_back();
                BoneIndex& boneIndex = indicies.mBoneIndices.back();
                boneIndex.mBone = mBoneCount;
                boneIndex.mWeight = weight.mWeight;
            }

            // contruct bone OBB from initial AABB.
            AABB initialAABB{topLeft, bottumRight};
            bone.mOBB = OBB{Basis{float3{1.0f, 0.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f}, float3{0.0f, 0.0f, 1.0f}},
                            float3{initialAABB.getSideLengths() / 2.0f},
                            float3{initialAABB.getMin()}};

            submesh.mSkeleton.push_back(bone);
            ++mBoneCount;
        }

        // Now generate bone weights offsets per vertex.
        for(uint32_t i = 0; i < mVertexCount; ++i)
        {
            const BoneIndicies& index = bonesPerVertex[i];

            uint2& indexSize = submesh.mBoneWeightsIndicies[i];
            indexSize.x = submesh.mBoneWeights.size();
            indexSize.y = index.mBoneIndices.size();

            submesh.mBoneWeights.insert(submesh.mBoneWeights.end(), index.mBoneIndices.begin(), index.mBoneIndices.end());
        }
    }
}


void StaticMesh::loadBlendMeshed(const aiMesh* mesh)
{
    for (uint32_t i = 0; i < mesh->mNumAnimMeshes; ++i)
    {
        mBlendMeshes.push_back(MeshBlend(mesh->mAnimMeshes[i]));
    }
}


uint32_t StaticMesh::getPrimitiveSize(const aiPrimitiveType primitiveType) const
{
		uint32_t primitiveElementSize = 0;

		switch(primitiveType)
		{
			case aiPrimitiveType::aiPrimitiveType_LINE:
			case aiPrimitiveType::aiPrimitiveType_POINT:
				primitiveElementSize = 1;
				break;

			case aiPrimitiveType::aiPrimitiveType_TRIANGLE:
				primitiveElementSize = 4;
				break;

			default:
				primitiveElementSize = 3;
		}

		return primitiveElementSize;
}


void StaticMesh::parseNode(const aiScene* scene,
                      const aiNode* node,
                      const aiMatrix4x4& parentTransofrmation,
                      const int vertAttributes)
{
    aiMatrix4x4 transformation = parentTransofrmation * node->mTransformation;

    for(uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* currentMesh = scene->mMeshes[node->mMeshes[i]];

        float4x4 transformationMatrix{};
        transformationMatrix[0][0] = transformation.a1; transformationMatrix[0][1] = transformation.b1;  transformationMatrix[0][2] = transformation.c1; transformationMatrix[0][3] = transformation.d1;
        transformationMatrix[1][0] = transformation.a2; transformationMatrix[1][1] = transformation.b2;  transformationMatrix[1][2] = transformation.c2; transformationMatrix[1][3] = transformation.d2;
        transformationMatrix[2][0] = transformation.a3; transformationMatrix[2][1] = transformation.b3;  transformationMatrix[2][2] = transformation.c3; transformationMatrix[2][3] = transformation.d3;
        transformationMatrix[3][0] = transformation.a4; transformationMatrix[3][1] = transformation.b4;  transformationMatrix[3][2] = transformation.c4; transformationMatrix[3][3] = transformation.d4;


        configure(scene, currentMesh, transformationMatrix, vertAttributes);
    }

    // Recurse through all child nodes
    for(uint32_t i = 0; i < node->mNumChildren; ++i)
    {
        parseNode(scene,
                  node->mChildren[i],
                  transformation,
                  vertAttributes);
    }
}


void StaticMesh::loadAnimations(const aiScene* scene)
{
    for(uint32_t i = 0; i < scene->mNumAnimations; ++i)
    {
        const aiAnimation* animation = scene->mAnimations[i];

        if (animation->mNumChannels > 0)
            mSkeletalAnimations.insert({ std::string(animation->mName.C_Str()), SkeletalAnimation(*this, animation, scene) });
        if (animation->mNumMorphMeshChannels > 0)
            mBlendAnimations.insert({ std::string(animation->mName.C_Str()), BlendMeshAnimation(animation, scene) });
        if (animation->mNumChannels == 0 && animation->mNumMorphMeshChannels == 0)
        {
            BELL_LOG_ARGS("Unsupported animation %s not loaded", animation->mName.C_Str())
        }
    }
}


void VertexBuffer::writeVertexVector4(const aiVector3D& vector)
{
    *reinterpret_cast<float*>(&mBuffer[mCurrentOffset]) = vector.x;
	*reinterpret_cast<float*>(&mBuffer[mCurrentOffset] + sizeof(float)) = vector.y;
	*reinterpret_cast<float*>(&mBuffer[mCurrentOffset] + 2 * sizeof(float)) = vector.z;
	*reinterpret_cast<float*>(&mBuffer[mCurrentOffset] + 3 * sizeof(float)) = 1.0f;

    mCurrentOffset += sizeof(float4);
}


void VertexBuffer::writeVertexVector2(const aiVector2D& vector)
{
    *reinterpret_cast<float*>(&mBuffer[mCurrentOffset]) = vector.x;
	*reinterpret_cast<float*>(&mBuffer[mCurrentOffset] + sizeof(float)) = vector.y;

    mCurrentOffset += sizeof(float2);
}


void VertexBuffer::writeVertexFloat(const float value)
{
	*reinterpret_cast<float*>(&mBuffer[mCurrentOffset]) = value;

    mCurrentOffset += sizeof(float);
}


void VertexBuffer::WriteVertexInt(const uint32_t value)
{
	*reinterpret_cast<uint32_t*>(&mBuffer[mCurrentOffset]) = value;

    mCurrentOffset += sizeof(uint32_t);
}

void VertexBuffer::WriteVertexChar4(const char4& value)
{
    *reinterpret_cast<int8_t*>(&mBuffer[mCurrentOffset]) = value.x;
    *reinterpret_cast<int8_t*>(&mBuffer[mCurrentOffset] + 1) = value.y;
    *reinterpret_cast<int8_t*>(&mBuffer[mCurrentOffset] + 2) = value.z;
    *reinterpret_cast<int8_t*>(&mBuffer[mCurrentOffset] + 3) = value.w;

    mCurrentOffset += sizeof(char4);
}


