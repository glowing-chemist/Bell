#include <limits>
#include <algorithm>
#include <numeric>

#include "Engine/BVH.hpp"
#include "Engine/GeomUtils.h"


static constexpr float NO_INTERSECTION = std::numeric_limits<float>::max();


template<typename T>
std::optional<T> BVH<T>::closestIntersection(const Ray& ray) const
{
    if(mRoot->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(mRoot->mLeafValue)
        return {*mRoot->mLeafValue};

    std::vector<std::pair<T, float>> leftChildren = getIntersectionsWithDistance(ray, mRoot->mLeft, std::numeric_limits<float>::infinity());
    std::vector<std::pair<T, float>> rightChildren = getIntersectionsWithDistance(ray, mRoot->mRight, std::numeric_limits<float>::infinity());

    leftChildren.insert(leftChildren.end(), rightChildren.begin(), rightChildren.end());

    std::pair<T, float> minPair = *std::min_element(leftChildren.begin(), leftChildren.end(),
                                                   [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second;});

    return minPair.first;
}


template<typename T>
std::vector<T>   BVH<T>::allIntersections(const Ray& ray) const
{
    if(mRoot->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(mRoot->mLeafValue)
        return {*mRoot->mLeafValue};

    std::vector<T> leftChildren;
    std::vector<T> rightChildren;

    if(mRoot->mLeft->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        leftChildren = getIntersections(ray, mRoot->mLeft);

    if(mRoot->mRight->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        rightChildren = getIntersections(ray, mRoot->mRight);

    leftChildren.insert(leftChildren.end(), rightChildren.begin(), rightChildren.end());

    return leftChildren;
}


template<typename T>
std::vector<T> BVH<T>::containedWithin(const Frustum& frustum, const std::unique_ptr<Node>& node, const EstimationMode estimationMode) const
{
	if (!frustum.isContainedWithin(node->mBoundingBox, estimationMode))
		return {};

	if (node->mLeafValue)
		return { *(node->mLeafValue) };

	std::vector<T> leftChildren;
	std::vector<T> rightChildren;

    if (frustum.isContainedWithin(node->mLeft->mBoundingBox, estimationMode))
        leftChildren = containedWithin(frustum, node->mLeft, estimationMode);

    if (frustum.isContainedWithin(node->mRight->mBoundingBox, estimationMode))
        rightChildren = containedWithin(frustum, node->mRight, estimationMode);

    leftChildren.insert(leftChildren.end(), rightChildren.begin(), rightChildren.end());

    return leftChildren;
}



template<typename T>
std::vector<T> BVH<T>::containedWithin(const Frustum& frustum, const EstimationMode estimationMode) const
{
	if (!frustum.isContainedWithin(mRoot->mBoundingBox, estimationMode))
		return {};

	if (mRoot->mLeafValue)
		return { *mRoot->mLeafValue };

	std::vector<T> leftChildren;
	std::vector<T> rightChildren;

    if (frustum.isContainedWithin(mRoot->mLeft->mBoundingBox, estimationMode))
        leftChildren = containedWithin(frustum, mRoot->mLeft, estimationMode);

    if (frustum.isContainedWithin(mRoot->mRight->mBoundingBox, estimationMode))
        rightChildren = containedWithin(frustum, mRoot->mRight, estimationMode);

    leftChildren.insert(leftChildren.end(), rightChildren.begin(), rightChildren.end());

    return leftChildren;
}


template<typename T>
std::vector<T> BVH<T>::getIntersections(const Ray& ray, const std::unique_ptr<Node>& node) const
{
    if(node->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(node->mLeafValue)
        return {*(node->mLeafValue)};

    std::vector<T> leftChildren;
    std::vector<T> rightChildren;

    if(node->mLeft->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        leftChildren = getIntersections(ray, node->mLeft);

    if(node->mRight->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        rightChildren = getIntersections(ray, node->mRight);

    leftChildren.insert(leftChildren.end(), rightChildren.begin(), rightChildren.end());

    return leftChildren;
}


template<typename T>
std::vector<std::pair<T, float>> BVH<T>::getIntersectionsWithDistance(const Ray& ray,
                                                               std::unique_ptr<Node>& node,
                                                               const float distance) const
{
    if(node->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(node->mLeafValue)
        return {{*(node->mLeafValue), distance}};

    std::vector<std::pair<T, float>> leftChildren;
    std::vector<std::pair<T, float>> rightChildren;

    const float leftDistance = node->mLeft->mBoundingBox.intersectionDistance(ray);
    if(leftDistance != NO_INTERSECTION)
        leftChildren = getIntersectionsWithDistance(ray, node->mLeft, leftDistance);

    const float rightDistance = node->mRight->mBoundingBox.intersectionDistance(ray);
    if(rightDistance != NO_INTERSECTION)
        rightChildren = getIntersectionsWithDistance(ray, node->mRight, rightDistance);

    leftChildren.insert(leftChildren.end(), rightChildren.begin(), rightChildren.end());

    return leftChildren;
}


template<typename T>
BVH<T> BVHFactory<T>::generateBVH()
{
	std::unique_ptr<typename BVH<T>::Node> root = partition(mBoundingBoxes, mRootBoundingBox);

    return BVH<T>(root);
}


template<typename T>
std::unique_ptr<typename BVH<T>::Node> BVHFactory<T>::partition(std::vector<std::pair<AABB, T>>& elements, const AABB& containingBox) const
{
    std::unique_ptr<typename BVH<T>::Node> node = std::make_unique<typename BVH<T>::Node>();
	node->mBoundingBox = containingBox;

	if (elements.size() > 2)
	{
		const auto[leftSplit, rightSplit] = splitAABB(containingBox);
		
		const auto leftBoxIter = std::partition(elements.begin(), elements.end(), [&leftSplit](const std::pair<AABB, T>& pair)
        { return leftSplit.contains(pair.first, EstimationMode::Under); });

		std::vector<std::pair<AABB, T>> rightChildren{ elements.begin(), leftBoxIter };
		std::vector<std::pair<AABB, T>> leftChildren{leftBoxIter, elements.end()};


        // Shrink
        AABB shrunkRight{{-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity() },
                    {std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity()}};

        for(const auto&[aabb, val] : rightChildren)
        {
            const float3 bottom = componentWiseMin(aabb.getBottom(), shrunkRight.getBottom());
            const float3 top = componentWiseMax(aabb.getTop(), shrunkRight.getTop());

            shrunkRight = AABB{top, bottom};
        }

        AABB shrunkLeft{{-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity() },
                    {std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity()}};

        for(const auto&[aabb, val] : leftChildren)
        {
            const float3 bottom = componentWiseMin(aabb.getBottom(), shrunkLeft.getBottom());
            const float3 top = componentWiseMax(aabb.getTop(), shrunkLeft.getTop());

            shrunkLeft = AABB{top, bottom};
        }

		if(!leftChildren.empty())
		{
			node->mLeft = partition(leftChildren, shrunkLeft);
		}

		if(!rightChildren.empty())
		{
			node->mRight = partition(rightChildren, shrunkRight);
		}
	}
	else
	{
        std::unique_ptr<typename BVH<T>::Node> leftNode = std::make_unique<typename BVH<T>::Node>();
        std::unique_ptr<typename BVH<T>::Node> rightNode = std::make_unique<typename BVH<T>::Node>();

		leftNode->mBoundingBox = elements[0].first;
		leftNode->mLeafValue = elements[0].second;

		if (elements.size() > 1)
		{
			rightNode->mBoundingBox = elements[1].first;
			rightNode->mLeafValue = elements[1].second;
		}

        node->mLeft = std::move(leftNode);
		node->mRight = std::move(rightNode);
	}

	return node;
}


// Split the AABB along its largest axis.
template<typename T>
std::pair<AABB, AABB> BVHFactory<T>::splitAABB(const AABB& aabb) const
{
	const Cube aabbCube = aabb.getCube();

	const float xLength = glm::length(aabbCube.mUpper1 - aabbCube.mUpper4);
	const float yLength = glm::length(aabbCube.mUpper1 - aabbCube.mLower1);
	const float zLength = glm::length(aabbCube.mUpper1 - aabbCube.mUpper2);

	const bool xMax = std::max(xLength, std::max(yLength, zLength)) == xLength;
	const bool yMax = std::max(yLength, std::max(xLength, zLength)) == yLength;

	if (xMax)
	{
		AABB first{ aabbCube.mUpper1, float3{ aabbCube.mLower3.x - (xLength / 2.0f), aabbCube.mLower3.y, aabbCube.mLower3.z } };
		AABB second{ float3{aabbCube.mUpper1.x + (xLength / 2.0f), aabbCube.mUpper1.y, aabbCube.mUpper1.z}, aabbCube.mLower3 };

		return { first, second };
	}
	else if (yMax)
	{
		AABB first{ aabbCube.mUpper1, float3{ aabbCube.mLower3.x, aabbCube.mLower3.y - (yLength / 2.0f), aabbCube.mLower3.z} };
		AABB second{ float3{aabbCube.mUpper1.x, aabbCube.mUpper1.y + (yLength / 2.0f), aabbCube.mUpper1.z}, aabbCube.mLower3 };

		return { first, second };
	}
	else
	{
		AABB first{ aabbCube.mUpper1, float3{ aabbCube.mLower3.x , aabbCube.mLower3.y, aabbCube.mLower3.z - (zLength / 2.0f) } };
        AABB second{ float3{aabbCube.mUpper1.x, aabbCube.mUpper1.y, aabbCube.mUpper1.z + (zLength / 2.0f)}, aabbCube.mLower3 };

		return { first, second };
	}
}


// Explicitly instantiate
#include "Engine/Scene.h"

template
class BVHFactory<Scene::MeshInstance*>;

template
class BVH<Scene::MeshInstance*>;


