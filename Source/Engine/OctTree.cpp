#include <limits>
#include <algorithm>
#include <numeric>
#include <map>
#include <set>

#include "Engine/OctTree.hpp"
#include "Engine/GeomUtils.h"


template<typename T>
void OctTree<T>::containedWithin(std::vector<T>& meshes, const Frustum& frustum, const typename OctTree<T>::Node& node, const Intersection nodeFlags) const
{
    if(nodeFlags & Intersection::Contains)
    {
        for (const auto& mesh : node.mValues)
        {
            meshes.push_back(mesh.mValue);
        }
    }
    else if(nodeFlags & Intersection::Partial)
    {
        for (const auto& mesh : node.mValues)
        {
            ++mTests;
            if (frustum.isContainedWithin(mesh.mBounds))
                meshes.push_back(mesh.mValue);
        }
    }
    else
    {
        return;
    }

    for (const auto& childIndex : node.mChildren)
	{
        if (childIndex != kInvalidNodeIndex)
        {
            const Node& childNode = getNode(childIndex);
            // no need to test subnodes if parent is completely contained.
            if(nodeFlags & Intersection::Contains)
            {
                containedWithin(meshes, frustum, childNode, nodeFlags);
            }
            else
            {
                ++mTests;
                const Intersection newFlags = frustum.isContainedWithin(childNode.mBoundingBox);
                if (newFlags)
                    containedWithin(meshes, frustum, childNode, newFlags);
            }
        }
	}
}



template<typename T>
std::vector<T> OctTree<T>::containedWithin(const Frustum& frustum) const
{
    mTests = 0;

    if (mRoot == kInvalidNodeIndex)
		return {};

    std::vector<T> uniqueMeshes{};

    const Node& root = getNode(mRoot);
    const Intersection initialFlags = frustum.isContainedWithin(root.mBoundingBox);
    containedWithin(uniqueMeshes, frustum, root, initialFlags);

    return uniqueMeshes;
}


template<typename T>
std::vector<T> OctTree<T>::getIntersections(const AABB& aabb) const
{
    mTests = 0;

	std::vector<T> intersections{};

    if(mRoot != kInvalidNodeIndex)
    {
        const Node& root = getNode(mRoot);
        const Intersection initialFlags = root.mBoundingBox.contains(aabb);
        getIntersections(aabb, root, intersections, initialFlags);
    }

    return intersections;
}


template<typename T>
void OctTree<T>::getIntersections(const AABB& aabb, const typename OctTree<T>::Node& node, std::vector<T>& intersections, const Intersection nodeFlags) const
{
    if(nodeFlags & Intersection::Partial)
    {
        for (const auto& mesh : node.mValues)
        {
            ++mTests;
            if (mesh.mBounds.contains(aabb))
                intersections.push_back(mesh.mValue);
        }
    }
    else
    {
        return;
    }

    for (const auto& childIndex : node.mChildren)
    {
        if (childIndex != kInvalidNodeIndex)
        {
            const Node childNode = getNode(childIndex);
            ++mTests;
            const Intersection newFlags = childNode.mBoundingBox.contains(aabb);
            if (newFlags)
                getIntersections(aabb, childNode, intersections, newFlags);
        }
    }
}


template<typename T>
OctTree<T> OctTreeFactory<T>::generateOctTree()
{
    const NodeIndex root = createSpacialSubdivisions(mRootBoundingBox, mBoundingBoxes);

    return OctTree<T>{root, mNodeStorage};
}


template<typename T>
NodeIndex OctTreeFactory<T>::createSpacialSubdivisions(const AABB& parentBox,
                                                       const std::vector<typename OctTree<T>::BoundedValue>& nodes)
{
    if (nodes.empty())
    {
        BELL_ASSERT(nodes.empty(), "Need to have placed all meshes")
        return kInvalidNodeIndex;
    }

    typename OctTree<T>::Node newNode{};
    newNode.mBoundingBox = parentBox;

    const float3 halfNodeSize = parentBox.getSideLengths() / 2.0f;
    std::vector<typename OctTree<T>::BoundedValue> unfittedNodes{};
	for (const auto& node : nodes)
	{
        const float3 size = node.mBounds.getSideLengths();
        if(size.x > halfNodeSize.x || size.y > halfNodeSize.y || size.z > halfNodeSize.z)
            newNode.mValues.push_back(node);
        else
            unfittedNodes.push_back(node);
	}

    const auto subSpaces = splitAABB(parentBox);

    uint32_t childCount = 0;
    std::map<uint32_t, uint32_t> unclaimedCount{};
    for (uint32_t i = 0; i < subSpaces.size(); ++i)
    {
        std::vector<typename OctTree<T>::BoundedValue> subSpaceNodes{};
        for(uint32_t j = 0; j < unfittedNodes.size(); ++j)
        {
            const auto& node = unfittedNodes[j];
            if(subSpaces[i].contains(node.mBounds) & Intersection::Contains)
            {
                subSpaceNodes.push_back(node);
            }
            else
            {
                uint32_t& counter = unclaimedCount[j];
                ++counter;
            }
        }

       const NodeIndex child = createSpacialSubdivisions(subSpaces[i], subSpaceNodes);
       if(child != kInvalidNodeIndex)
            ++childCount;
       newNode.mChildren[i] = child;
    }
    newNode.mChildCount = childCount;

    for(const auto&[idx, count] : unclaimedCount)
    {
        BELL_ASSERT(count <= 8, "Incorrect map lookup")
        if(count == 8)
            newNode.mValues.push_back(unfittedNodes[idx]);
    }

    if(newNode.mValues.empty() && newNode.mChildCount == 0)
        return kInvalidNodeIndex;
    else
        return addNode(newNode);
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


