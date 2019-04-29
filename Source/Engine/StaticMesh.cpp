#include "Engine/StaticMesh.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

StaticMesh::StaticMesh(const std::string& path)
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
                primitiveElementSize = 3;
                break;

            default:
                primitiveElementSize = 3;
        }

        return primitiveElementSize;
    }();

    const uint32_t vertexStride =   (mesh->HasPositions() ? primitiveSize * 4 : 0) +
                                    (mesh->HasTextureCoords(0) ? 8 : 0) +
                                    (mesh->HasNormals() ? 16 : 0) +
                                    (mesh->HasVertexColors(0) ? 4 : 0);

    mIndexData.resize(mesh->mFaces[0].mNumIndices);
    mVertexData.resize(mesh->mNumFaces * vertexStride);

    // Copy the index buffer data.
    std::memcpy(mIndexData.data(), mesh->mFaces[0].mIndices, mesh->mFaces[0].mNumIndices * sizeof (uint32_t));

    uint32_t currentOffset = 0;

    // Copy the vertex buffer data.
    for(uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
        if(mesh->HasPositions())
        {
            writeVertexVector4(mesh->mVertices[i], currentOffset);
            currentOffset += 16;
        }

        if(mesh->HasTextureCoords(0))
        {
            writeVertexVector2({mesh->mTextureCoords[i][0].x, mesh->mTextureCoords[i][0].y}, currentOffset);
            currentOffset += 8;
        }

        if(mesh->HasNormals())
        {
            writeVertexVector4(mesh->mNormals[i], currentOffset);
            currentOffset += 16;
        }

        if(mesh->HasVertexColors(0))
        {
            writeVertexVector4({mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b}, currentOffset);
            currentOffset += 16;
        }
    }
}


void StaticMesh::writeVertexVector4(const aiVector3D& vector, const uint32_t startOffset)
{
    *reinterpret_cast<float*>(&mVertexData[startOffset]) = vector.x;
    *reinterpret_cast<float*>(&mVertexData[startOffset + sizeof(float)]) = vector.y;
    *reinterpret_cast<float*>(&mVertexData[startOffset + sizeof(float) * 2]) = vector.z;
    *reinterpret_cast<float*>(&mVertexData[startOffset + sizeof(float) * 3]) = 1.0f;
}


void StaticMesh::writeVertexVector2(const aiVector2D& vector, const uint32_t startOffset)
{
    *reinterpret_cast<float*>(&mVertexData[startOffset]) = vector.x;
    *reinterpret_cast<float*>(&mVertexData[startOffset + sizeof(float)]) = vector.y;
}

