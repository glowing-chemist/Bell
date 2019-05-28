#include "Engine/StaticMesh.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <limits>


StaticMesh::StaticMesh(const std::string& path, const int vertAttributes)
{
    Assimp::Importer importer;

    const aiScene* model = importer.ReadFile(path.c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
											 aiProcess_GenNormals);

    const aiMesh* mesh = model->mMeshes[0];

    const unsigned int primitiveType = mesh->mPrimitiveTypes;

    const uint32_t primitiveSize = [primitiveType] ()
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
    }();

	const bool positionNeeded = mesh->HasPositions() && ((vertAttributes & VertexAttributes::Position2 ||
													 vertAttributes & VertexAttributes::Position3 ||
													 vertAttributes & VertexAttributes::Position4));

	const bool UVNeeded = mesh->HasTextureCoords(0) && (vertAttributes & VertexAttributes::TextureCoordinates);

	const bool normalsNeeded = mesh->HasNormals() && (vertAttributes & VertexAttributes::Normals);

	const bool albedoNeeded = mesh->HasVertexColors(0) && (vertAttributes & VertexAttributes::Aledo);

	const bool materialNeeded = model->HasMaterials() && (vertAttributes & VertexAttributes::Material);

	const uint32_t vertexStride =   (positionNeeded ? primitiveSize * 1 : 0) +
									(UVNeeded ? 2 : 0) +
									(normalsNeeded ? 4 : 0) +
									(albedoNeeded ? 4 : 0) +
									(materialNeeded != 0 ? 1 : 0);

    // assume triangles atm
    mIndexData.resize(mesh->mNumFaces * mesh->mFaces[0].mNumIndices);
    mVertexData.resize(mesh->mNumVertices * vertexStride);

    // Copy the index data
    uint32_t currentIndex = 0;
    for(uint32_t i = 0; i < mesh->mNumFaces; ++i, currentIndex += 3)
    {
        mIndexData[currentIndex] = mesh->mFaces[i].mIndices[0];
        mIndexData[currentIndex + 1] = mesh->mFaces[i].mIndices[1];
        mIndexData[currentIndex + 2] = mesh->mFaces[i].mIndices[2];
    }

    uint32_t currentOffset = 0;

    // Copy the vertex buffer data.
    for(uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
		if(positionNeeded)
        {
            writeVertexVector4(mesh->mVertices[i], currentOffset);
            currentOffset += 4;
        }

		if(UVNeeded)
        {
            writeVertexVector2({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}, currentOffset);
            currentOffset += 2;
        }

		if(normalsNeeded)
        {
            writeVertexVector4(mesh->mNormals[i], currentOffset);
            currentOffset += 4;
        }

		if(albedoNeeded)
        {
            writeVertexVector4({mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b}, currentOffset);
            currentOffset += 4;
        }

		if(materialNeeded)
		{
			writeVertexFloat(mesh->mMaterialIndex, currentOffset);
			currentOffset += 1;
		}
    }
}


void StaticMesh::writeVertexVector4(const aiVector3D& vector, const uint32_t startOffset)
{
    *reinterpret_cast<float*>(&mVertexData[startOffset]) = vector.x;
    *reinterpret_cast<float*>(&mVertexData[startOffset] + 1) = vector.y;
    *reinterpret_cast<float*>(&mVertexData[startOffset] + 2) = vector.z;
    *reinterpret_cast<float*>(&mVertexData[startOffset] + 3) = 1.0f;
}


void StaticMesh::writeVertexVector2(const aiVector2D& vector, const uint32_t startOffset)
{
    *reinterpret_cast<float*>(&mVertexData[startOffset]) = vector.x;
    *reinterpret_cast<float*>(&mVertexData[startOffset] + 1) = vector.y;
}


void StaticMesh::writeVertexFloat(const float value, const uint32_t startOffset)
{
	*reinterpret_cast<float*>(&mVertexData[startOffset]) = value;
}


