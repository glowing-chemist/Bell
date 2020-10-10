#include "Engine/StaticMesh.h"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

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


StaticMesh::StaticMesh(const std::string& path, const int vertAttributes) :
	mVertexData{},
	mIndexData{},
    mAABB{},
    mPassTypes{static_cast<uint64_t>(PassType::EditorDefault)},
	mVertexCount(0),
    mVertexAttributes(vertAttributes),
    mVertexStride(0)
{
	Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path.c_str(),
											 aiProcess_Triangulate |
											 aiProcess_JoinIdenticalVertices |
											 aiProcess_GenNormals |
											 aiProcess_FlipUVs);

    BELL_ASSERT(scene->mNumMeshes == 1, "This files containes more than 1 mesh, which one is loaded is undefined.")

    const aiMesh* mesh = scene->mMeshes[0];

    configure(scene, mesh, vertAttributes);
}


StaticMesh::StaticMesh(const aiScene *scene, const aiMesh* mesh, const int vertexAttributes) :
    mVertexAttributes(vertexAttributes)
{
    configure(scene, mesh, vertexAttributes);
}


void StaticMesh::configure(const aiScene* scene, const aiMesh* mesh, const int vertAttributes)
{
    const unsigned int primitiveType = mesh->mPrimitiveTypes;

    const uint32_t primitiveSize = getPrimitiveSize(static_cast<aiPrimitiveType>(primitiveType));

    const bool positionNeeded = mesh->HasPositions() && ((vertAttributes & VertexAttributes::Position2 ||
                                                     vertAttributes & VertexAttributes::Position3 ||
                                                     vertAttributes & VertexAttributes::Position4));

    const bool UVNeeded = mesh->HasTextureCoords(0) && (vertAttributes & VertexAttributes::TextureCoordinates);

    const bool normalsNeeded = mesh->HasNormals() && (vertAttributes & VertexAttributes::Normals);

    const bool albedoNeeded = (mesh->GetNumColorChannels() > 0) && (vertAttributes & VertexAttributes::Albedo);

    // relys on float and MaterialID beingn the same size (should always be true).
    static_assert(sizeof(float) == sizeof(uint32_t), "Material ID doesn't match sizeof(float");
    const uint32_t vertexStride =   ((positionNeeded ? primitiveSize : 0) +
                                    ((vertAttributes & VertexAttributes::TextureCoordinates) ? 2 : 0) +
                                    (normalsNeeded ? 1 : 0) +
                                    ((vertAttributes & VertexAttributes::Albedo) ? 1 : 0)) * sizeof(float);

	mVertexStride = vertexStride;
	mVertexCount = mesh->mNumVertices;

    // assume triangles atm
    mIndexData.reserve(mesh->mNumFaces * mesh->mFaces[0].mNumIndices);
    mVertexData.setSize(mesh->mNumVertices * vertexStride);

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

    mAABB = AABB{topLeft, bottumRight};

    loadSkeleton(mesh);
    loadBlendMeshed(mesh);

    if(hasAnimations())
    {
        // Load animations.
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
}


void StaticMesh::loadSkeleton(const aiMesh* mesh)
{
    if(mesh->mNumBones > 0)
    {
        mSkeleton.reserve(mesh->mNumBones);
        std::vector<BoneIndicies> bonesPerVertex;
        bonesPerVertex.resize(mVertexCount);
        mBoneWeightsIndicies.resize(mVertexCount);

        for(uint32_t i = 0; i < mesh->mNumBones; ++i)
        {
            const aiBone* assimpBone = mesh->mBones[i];
            if(assimpBone->mNumWeights == 0)
                continue;

            Bone bone{};
            bone.mName = assimpBone->mName.C_Str();
            bone.mInverseBindPose = aiMatrix4x4ToFloat4x4(assimpBone->mOffsetMatrix);

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
                BELL_ASSERT(indicies.mUsedBones < 25, "Only 25 or less bones per vertex are currently supported")
                const uint8_t index = indicies.mUsedBones++;
                BoneIndex& boneIndex = indicies.mBoneIndices[index];
                boneIndex.mBone = mSkeleton.size();
                boneIndex.mWeight = weight.mWeight;
            }

            // contruct bone OBB from initial AABB.
            AABB initialAABB{topLeft, bottumRight};
            bone.mOBB = OBB{Basis{float3{1.0f, 0.0f, 0.0f}, float3{0.0f, 1.0f, 0.0f}, float3{0.0f, 0.0f, 1.0f}},
                            float3{initialAABB.getSideLengths() / 2.0f},
                            float3{initialAABB.getBottom()}};

            mSkeleton.push_back(bone);
        }

        // Now generate bone weights offsets per vertex.
        for(uint32_t i = 0; i < mVertexCount; ++i)
        {
            const BoneIndicies& index = bonesPerVertex[i];

            uint2& indexSize = mBoneWeightsIndicies[i];
            indexSize.x = mBoneWeights.size();
            indexSize.y = index.mUsedBones;

            mBoneWeights.insert(mBoneWeights.end(), &index.mBoneIndices[0], &index.mBoneIndices[index.mUsedBones]);
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


