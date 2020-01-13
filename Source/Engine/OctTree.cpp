#include <limits>
#include <algorithm>
#include <numeric>
#include <set>

#include "Engine/OctTree.hpp"
#include "Engine/GeomUtils.h"


static constexpr float NO_INTERSECTION = std::numeric_limits<float>::max();


template<typename T>
std::vector<T> OctTree<T>::containedWithin(const Frustum& frustum, const std::unique_ptr<typename OctTree<T>::Node>& node, const EstimationMode estimationMode) const
{
	if (frustum.isContainedWithin(node->mBoundingBox, EstimationMode::Under))
		return node->mValues;

	std::set<T> values{};

	for (const auto& childNode : node->mChildren)
	{
		if (!childNode)
		{
			values.insert(node->mValues.begin(), node->mValues.end());
		}
		else if (frustum.isContainedWithin(childNode->mBoundingBox, estimationMode))
		{
			values.insert(node->mValues.begin(), node->mValues.end());
		}
		else
		{
			for (const auto& subNode : node->mChildren)
			{
				const auto v = containedWithin(frustum, subNode, estimationMode);
				values.insert(v.begin(), v.end());
			} 
		}
	}

	return std::vector<T>{values.begin(), values.end()};
}



template<typename T>
std::vector<T> OctTree<T>::containedWithin(const Frustum& frustum, const EstimationMode estimationMode) const
{
	if (!mRoot)
		return {};

#if 1
	std::vector<T> meshes{};

	for (const auto& mesh : mRoot->mValues)
	{
		if (frustum.isContainedWithin(mesh->mMesh->getAABB() * mesh->mTransformation, estimationMode))
		{
			meshes.push_back(mesh);
		}
	}

	return meshes;
#else
	return containedWithin(frustum, mRoot, estimationMode);
#endif
}


template<typename T>
OctTree<T> OctTreeFactory<T>::generateOctTree(const uint32_t subdivisions)
{
	auto root = createSpacialSubdivisions(subdivisions, mRootBoundingBox, mBoundingBoxes);

	return OctTree<T>{root};
}


template<typename T>
std::unique_ptr<typename OctTree<T>::Node> OctTreeFactory<T>::createSpacialSubdivisions(const uint32_t subdivisions, const AABB& parentBox, const std::vector<BuilderNode>& nodes)
{
	auto newNode = std::make_unique<typename OctTree<T>::Node>();
	newNode->mBoundingBox = parentBox;

	for (const auto& node : nodes)
	{
		if (parentBox.contains(node.mBoundingBox, EstimationMode::Over))
		{
			newNode->mValues.push_back(node.mValue);
		}
	}

	if (subdivisions > 1)
	{
		const auto subSpaces = splitAABB(parentBox);

		for (uint32_t i = 0; i < subSpaces.size(); ++i)
		{
			newNode->mChildren[i] = createSpacialSubdivisions(subdivisions - 1, subSpaces[i], nodes);
		}
	}

	return newNode;
}




template<typename T>
std::array<AABB, 8> OctTreeFactory<T>::splitAABB(const AABB& aabb) const
{
	Cube cube = aabb.getCube();

	const float3 diagonal = cube.mLower3 - cube.mUpper1;
	const float3 centre = cube.mUpper1 + (diagonal / 2.0f);

	// Top layer
	const AABB first(cube.mUpper1, centre);
	const AABB second(cube.mUpper1 + float3(0.0f, 0.0f, diagonal.z / 2.0f), centre + float3(0.0f, 0.0f, diagonal.z / 2.0f));
	const AABB third(cube.mUpper1 + float3(diagonal.x / 2.0f, 0.0f, 0.0f), centre + float3(diagonal.x / 2.0f, 0.0f, 0.0f));
	const AABB fourth(cube.mUpper1 + float3(diagonal.x / 2.0f, 0.0f, diagonal.z / 2.0f), centre + float3(diagonal.x / 2.0f, 0.0f, diagonal.z / 2.0f));

	// Bottom layer
	const AABB fith(cube.mUpper1 + float3(0.0f, diagonal.y / 2.0f, 0.0f), centre + float3(0.0f, diagonal.y / 2.0f, 0.0f));
	const AABB sixth(cube.mUpper1 + float3(0.0f, diagonal.y / 2.0f, diagonal.z / 2.0f), centre + float3(0.0f, diagonal.y / 2.0f, diagonal.z / 2.0f));
	const AABB seventh(cube.mUpper1 + float3(diagonal.x / 2.0f, diagonal.y / 2.0f, 0.0f), centre + float3(diagonal.x / 2.0f, diagonal.y / 2.0f, 0.0f));
	const AABB eighth(cube.mUpper1 + float3(diagonal.x / 2.0f, diagonal.y / 2.0f, diagonal.z / 2.0f), cube.mUpper1 + diagonal);

	return { first, second, third, fourth,
			fith, sixth, seventh, eighth };

}


// Explicitly instantiate
#include "Engine/Scene.h"

template
class OctTreeFactory<Scene::MeshInstance*>;

template
class OctTree<Scene::MeshInstance*>;


