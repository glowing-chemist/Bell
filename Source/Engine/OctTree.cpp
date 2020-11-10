#include <limits>
#include <algorithm>
#include <numeric>
#include <set>

#include "Engine/OctTree.hpp"
#include "Engine/GeomUtils.h"


// Can be enabled for small scenes where the cost of octree traversal traversal is greater
// than just looping over all meshes.
#define SMALL_SCENE_OPTIMISATION 0


template<typename T>
void OctTree<T>::containedWithin(std::set<T>& meshes, const Frustum& frustum, const std::unique_ptr<typename OctTree<T>::Node>& node, const EstimationMode estimationMode) const
{
	if (frustum.isContainedWithin(node->mBoundingBox, EstimationMode::Under))
	{
        for(const auto node : node->mValues)
            meshes.insert(node.mValue);
		return;
	}

	for (const auto& childNode : node->mChildren)
	{
        if (childNode)
        {
            if (frustum.isContainedWithin(childNode->mBoundingBox, estimationMode))
                containedWithin(meshes, frustum, childNode, estimationMode);
        }
        else
		{
			if (frustum.isContainedWithin(node->mBoundingBox, estimationMode))
			{
				for (const auto& mesh : node->mValues)
				{
                    if (frustum.isContainedWithin(mesh.mBounds , estimationMode))
                        meshes.insert(mesh.mValue);
				}
			}
			return;
		}
	}
}



template<typename T>
std::vector<T> OctTree<T>::containedWithin(const Frustum& frustum, const EstimationMode estimationMode) const
{
	if (!mRoot)
		return {};

    std::set<T> meshes{};

	containedWithin(meshes, frustum, mRoot, estimationMode);

    std::vector<T> uniqueMeshes(meshes.begin(), meshes.end());

    return uniqueMeshes;
}


template<typename T>
std::vector<T> OctTree<T>::getIntersections(const AABB& aabb) const
{
	std::vector<T> intersections{};

	if(mRoot != nullptr)
		getIntersections(aabb, mRoot, intersections);

    return intersections;
}


template<typename T>
void OctTree<T>::getIntersections(const AABB& aabb, const std::unique_ptr<typename OctTree<T>::Node>& node, std::vector<T>& intersections) const
{
	if (aabb.contains(node->mBoundingBox, EstimationMode::Under))
	{
        for(const auto node : node->mValues)
            intersections.push_back(node.mValue);
		return;
	}

	for (const auto& childNode : node->mChildren)
	{
		if (!childNode)
		{
			if (node->mBoundingBox.contains(aabb, EstimationMode::Over))
			{
				for (const auto& mesh : node->mValues)
				{
                    if (aabb.contains(mesh.mBounds, EstimationMode::Over))
                        intersections.push_back(mesh.mValue);
				}
			}
			return;
		}
		else
		{
			for (const auto& subNode : node->mChildren)
			{
				if (subNode->mBoundingBox.contains(aabb, EstimationMode::Over))
					getIntersections(aabb, subNode, intersections);
			}
		}
	}
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

	if (nodes.empty())
		return newNode;

	for (const auto& node : nodes)
	{
		if (parentBox.contains(node.mBoundingBox, EstimationMode::Over))
		{
            newNode->mValues.push_back({node.mBoundingBox, node.mValue});
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

	const float4 diagonal = cube.mLower3 - cube.mUpper1;
	const float4 centre = cube.mUpper1 + (diagonal / 2.0f);

	// Top layer
	const AABB first(cube.mUpper1, centre);
	const AABB second(cube.mUpper1 + float4(0.0f, 0.0f, diagonal.z / 2.0f, 1.0f), centre + float4(0.0f, 0.0f, diagonal.z / 2.0f, 1.0f));
	const AABB third(cube.mUpper1 + float4(diagonal.x / 2.0f, 0.0f, 0.0f, 1.0f), centre + float4(diagonal.x / 2.0f, 0.0f, 0.0f, 1.0f));
	const AABB fourth(cube.mUpper1 + float4(diagonal.x / 2.0f, 0.0f, diagonal.z / 2.0f, 1.0f), centre + float4(diagonal.x / 2.0f, 0.0f, diagonal.z / 2.0f, 1.0f));

	// Bottom layer
	const AABB fith(cube.mUpper1 + float4(0.0f, diagonal.y / 2.0f, 0.0f, 1.0f), centre + float4(0.0f, diagonal.y / 2.0f, 0.0f, 1.0f));
	const AABB sixth(cube.mUpper1 + float4(0.0f, diagonal.y / 2.0f, diagonal.z / 2.0f, 1.0f), centre + float4(0.0f, diagonal.y / 2.0f, diagonal.z / 2.0f, 1.0f));
	const AABB seventh(cube.mUpper1 + float4(diagonal.x / 2.0f, diagonal.y / 2.0f, 0.0f, 1.0f), centre + float4(diagonal.x / 2.0f, diagonal.y / 2.0f, 0.0f, 1.0f));
	const AABB eighth(centre, cube.mLower3);

	return { first, second, third, fourth,
			fith, sixth, seventh, eighth };

}


// Explicitly instantiate
#include "Engine/Scene.h"

template
class OctTreeFactory<MeshInstance*>;

template
class OctTree<MeshInstance*>;


