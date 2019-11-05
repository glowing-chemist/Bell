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
std::vector<T> BVH<T>::containedWithin(const Frustum& frustum, const std::unique_ptr<typename BVH<T>::Node>& node, const EstimationMode estimationMode) const
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
    if (!mRoot)
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
std::vector<T> BVH<T>::getIntersections(const Ray& ray, const std::unique_ptr<typename BVH<T>::Node>& node) const
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
std::unique_ptr<typename BVH<T>::Node> BVHFactory<T>::partition(std::vector<typename BVHFactory<T>::BuilderNode>& elements, const AABB& containingBox) const
{
    std::unique_ptr<typename BVH<T>::Node> node = std::make_unique<typename BVH<T>::Node>();
	node->mBoundingBox = containingBox;

	if (elements.size() > 2)
	{
		auto[leftSplit, rightSplit] = splitAABB(containingBox, elements);
		

		std::vector<typename BVHFactory<T>::BuilderNode>& leftChildren = leftSplit.second;
		std::vector<typename BVHFactory<T>::BuilderNode>& rightChildren = rightSplit.second;


        // Shrink
        AABB shrunkRight{{std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity() },
                    {-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()}};

        for(const auto&[aabb, val] : rightChildren)
        {
            const float3 bottom = componentWiseMax(aabb.getBottom(), shrunkRight.getBottom());
            const float3 top = componentWiseMin(aabb.getTop(), shrunkRight.getTop());

            shrunkRight = AABB{top, bottom};
        }

        AABB shrunkLeft{{std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity(),
                    std::numeric_limits<float>::infinity() },
                    {-std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity(),
                    -std::numeric_limits<float>::infinity()}};

        for(const auto&[aabb, val] : leftChildren)
        {
            const float3 bottom = componentWiseMax(aabb.getBottom(), shrunkLeft.getBottom());
            const float3 top = componentWiseMin(aabb.getTop(), shrunkLeft.getTop());

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

		leftNode->mBoundingBox = elements[0].mBox;
		leftNode->mLeafValue = elements[0].mValue;

		if (elements.size() > 1)
		{
			rightNode->mBoundingBox = elements[1].mBox;
			rightNode->mLeafValue = elements[1].mValue;
		}

        node->mLeft = std::move(leftNode);
		node->mRight = std::move(rightNode);
	}

	return node;
}


// Split the AABB along its largest axis.
template<typename T>
std::pair<std::pair<AABB, std::vector<typename BVHFactory<T>::BuilderNode>>, std::pair<AABB, std::vector<typename BVHFactory<T>::BuilderNode>>>
	BVHFactory<T>::splitAABB(const AABB& aabb, std::vector<typename BVHFactory<T>::BuilderNode>& nodes) const
{
	const float3 bottomRight = aabb.getBottom();
	const float3 topLeft = aabb.getTop();

	const float3 diagonal = bottomRight - topLeft;

	const float xLength = diagonal.x;
	const float yLength = diagonal.y;
	const float zLength = diagonal.z;
	
	const bool xMax = std::max(xLength, std::max(yLength, zLength)) == xLength;
	const bool yMax = std::max(yLength, std::max(xLength, zLength)) == yLength;
	const bool zMax = std::max(zLength, std::max(xLength, yLength)) == zLength;
	
	const float3 splitDirection = { xMax ? 1.0f : 0.0f, yMax ? 1.0f : 0.0f, zMax ? 1.0f : 0.0f };
	const float3 invertedSplit = -(splitDirection - 1.0f);

	float3 averageDistance{0.0f, 0.0f, 0.0f};
	for (const auto& node : nodes)
	{
		averageDistance += node.mBox.getBottom() - topLeft;
	}

	averageDistance /= float(nodes.size());

	averageDistance *= splitDirection;

	const AABB leftBox{topLeft, topLeft + averageDistance + (invertedSplit * diagonal)};
	const AABB rightBox{ topLeft + averageDistance, bottomRight};

	const auto mid = std::partition(nodes.begin(), nodes.end(), [&leftBox](const auto& node)
		{
			return leftBox.contains(node.mBox, EstimationMode::Over);
		});

	return std::make_pair(std::make_pair(leftBox, std::vector<typename BVHFactory<T>::BuilderNode>{nodes.begin(), mid}), std::make_pair(rightBox, std::vector<typename BVHFactory<T>::BuilderNode>{mid, nodes.end()}));
}


// Explicitly instantiate
#include "Engine/Scene.h"

template
class BVHFactory<Scene::MeshInstance*>;

template
class BVH<Scene::MeshInstance*>;


