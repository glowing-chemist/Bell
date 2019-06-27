#include "Engine/StaticMesh.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"

#include <limits>


StaticMesh::StaticMesh(const std::string& path, const int vertAttributes) :
	mVertexData{},
	mIndexData{},
	mAABB{}
{
    Assimp::Importer importer;

    const aiScene* model = importer.ReadFile(path.c_str(),
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
											 aiProcess_GenNormals |
											 aiProcess_FlipUVs);

    const aiMesh* mesh = model->mMeshes[0];

    const unsigned int primitiveType = mesh->mPrimitiveTypes;

	const uint32_t primitiveSize = getPrimitiveSize(static_cast<aiPrimitiveType>(primitiveType));

	const bool positionNeeded = mesh->HasPositions() && ((vertAttributes & VertexAttributes::Position2 ||
													 vertAttributes & VertexAttributes::Position3 ||
													 vertAttributes & VertexAttributes::Position4));

	const bool UVNeeded = mesh->HasTextureCoords(0) && (vertAttributes & VertexAttributes::TextureCoordinates);

	const bool normalsNeeded = mesh->HasNormals() && (vertAttributes & VertexAttributes::Normals);

	const bool albedoNeeded = mesh->HasVertexColors(0) && (vertAttributes & VertexAttributes::Albedo);

	const bool materialNeeded = model->HasMaterials() && (vertAttributes & VertexAttributes::Material);

	// relys on float and MaterialID beingn the same size (should always be true).
	static_assert(sizeof(float) == sizeof(uint32_t), "Material ID doesn't match sizeof(float");
	const uint32_t vertexStride =   ((positionNeeded ? primitiveSize : 0) +
									(UVNeeded ? 2 : 0) +
									(normalsNeeded ? 4 : 0) +
									(albedoNeeded ? 4 : 0) +
									(materialNeeded != 0 ? 1 : 0)) * sizeof(float);

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

	float3 topLeft{0.0f, 0.0f, 0.0f};
	float3 bottumRight{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};

    // Copy the vertex buffer data.
    for(uint32_t i = 0; i < mesh->mNumVertices; ++i)
    {
		if(positionNeeded)
        {
            writeVertexVector4(mesh->mVertices[i], currentOffset);
			currentOffset += 4 * sizeof(float);

			// update the AABB positions
			topLeft = componentWiseMin(topLeft, float3{	mesh->mVertices[i].x,
														mesh->mVertices[i].y,
														mesh->mVertices[i].z});

			bottumRight = componentWiseMax(bottumRight, float3{	mesh->mVertices[i].x,
																mesh->mVertices[i].y,
																mesh->mVertices[i].z});
        }

		if(UVNeeded)
        {
            writeVertexVector2({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}, currentOffset);
			currentOffset += 2 * sizeof(float);
        }

		if(normalsNeeded)
        {
            writeVertexVector4(mesh->mNormals[i], currentOffset);
			currentOffset += 4 * sizeof(float);
        }

		if(albedoNeeded)
        {
            writeVertexVector4({mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b}, currentOffset);
			currentOffset += 4 * sizeof(float);
        }

		if(materialNeeded)
		{
			writeVertexFloat(mesh->mMaterialIndex, currentOffset);
			currentOffset += sizeof(uint32_t);
		}
    }

	mAABB = AABB{topLeft, bottumRight};
}


StaticMesh::StaticMesh(const std::string& path, const int vertAttributes, const uint32_t materialID) :
	mVertexData{},
	mIndexData{},
	mAABB{}
{
	Assimp::Importer importer;

	const aiScene* model = importer.ReadFile(path.c_str(),
											 aiProcess_Triangulate |
											 aiProcess_JoinIdenticalVertices |
											 aiProcess_GenNormals |
											 aiProcess_FlipUVs);

	const aiMesh* mesh = model->mMeshes[0];

	const unsigned int primitiveType = mesh->mPrimitiveTypes;

	const uint32_t primitiveSize = getPrimitiveSize(static_cast<aiPrimitiveType>(primitiveType));

	const bool positionNeeded = mesh->HasPositions() && ((vertAttributes & VertexAttributes::Position2 ||
													 vertAttributes & VertexAttributes::Position3 ||
													 vertAttributes & VertexAttributes::Position4));

	const bool UVNeeded = mesh->HasTextureCoords(0) && (vertAttributes & VertexAttributes::TextureCoordinates);

	const bool normalsNeeded = mesh->HasNormals() && (vertAttributes & VertexAttributes::Normals);

	const bool albedoNeeded = mesh->HasVertexColors(0) && (vertAttributes & VertexAttributes::Albedo);

	const uint32_t vertexStride =   ((positionNeeded ? primitiveSize * 1 : 0) +
									(UVNeeded ? 2 : 0) +
									(normalsNeeded ? 4 : 0) +
									(albedoNeeded ? 4 : 0) +
									1) * sizeof(float);

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

	float3 topLeft{0.0f, 0.0f, 0.0f};
	float3 bottumRight{std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity()};


	// Copy the vertex buffer data.
	for(uint32_t i = 0; i < mesh->mNumVertices; ++i)
	{
		if(positionNeeded)
		{
			writeVertexVector4(mesh->mVertices[i], currentOffset);
			currentOffset += 4 * sizeof(float);

			// update the AABB positions
			topLeft = componentWiseMin(topLeft, float3{	mesh->mVertices[i].x,
														mesh->mVertices[i].y,
														mesh->mVertices[i].z});

			bottumRight = componentWiseMax(bottumRight, float3{	mesh->mVertices[i].x,
																mesh->mVertices[i].y,
																mesh->mVertices[i].z});
		}

		if(UVNeeded)
		{
			writeVertexVector2({mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y}, currentOffset);
			currentOffset += 2 * sizeof(float);
		}

		if(normalsNeeded)
		{
			writeVertexVector4(mesh->mNormals[i], currentOffset);
			currentOffset += 4 * sizeof(float);
		}

		if(albedoNeeded)
		{
			writeVertexVector4({mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b}, currentOffset);
			currentOffset += 4 * sizeof(float);
		}

		WriteVertexInt(materialID, currentOffset);
		currentOffset += sizeof(uint32_t);
	}

	mAABB = AABB{topLeft, bottumRight};
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


void StaticMesh::writeVertexVector4(const aiVector3D& vector, const uint32_t startOffset)
{
    *reinterpret_cast<float*>(&mVertexData[startOffset]) = vector.x;
	*reinterpret_cast<float*>(&mVertexData[startOffset] + sizeof(float)) = vector.y;
	*reinterpret_cast<float*>(&mVertexData[startOffset] + 2 * sizeof(float)) = vector.z;
	*reinterpret_cast<float*>(&mVertexData[startOffset] + 3 * sizeof(float)) = 1.0f;
}


void StaticMesh::writeVertexVector2(const aiVector2D& vector, const uint32_t startOffset)
{
    *reinterpret_cast<float*>(&mVertexData[startOffset]) = vector.x;
	*reinterpret_cast<float*>(&mVertexData[startOffset] + sizeof(float)) = vector.y;
}


void StaticMesh::writeVertexFloat(const float value, const uint32_t startOffset)
{
	*reinterpret_cast<float*>(&mVertexData[startOffset]) = value;
}


void StaticMesh::WriteVertexInt(const uint32_t value, const uint32_t startOffset)
{
	*reinterpret_cast<uint32_t*>(&mVertexData[startOffset]) = value;
}

