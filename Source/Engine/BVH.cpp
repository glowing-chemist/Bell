#include <algorithm>

#include "Engine/BVH.hpp"


static constexpr float NO_INTERSECTION = std::numeric_limits<float>::max();


template<typename T>
std::optional<T> BVH<T>::closestIntersection(const Ray& ray) const
{
    if(mRoot->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(mRoot->mLeafValue)
        return {*mRoot->mLeafValue};

    std::vector<std::pair<T, float>> leftChildren = getIntersections(ray, mRoot->Left);
    std::vector<std::pair<T, float>> rightChildren = getIntersections(ray, mRoot->mRight);

    leftChildren.insert(leftChildren.back(), rightChildren.begin(), rightChildren.end());

    std::pair<T, float>& minPair = std::min_element(leftChildren.begin(), leftChildren.end(),
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
        leftChildren = getIntersections(ray, mRoot->Left);

    if(mRoot->mRight->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        rightChildren = getIntersections(ray, mRoot->mRight);

    return leftChildren.insert(leftChildren.back(), rightChildren.begin(), rightChildren.end());
}


template<typename T>
std::vector<T> BVH<T>::containedWithin(const Frustum& frustum, const EstimationMode estimationMode) const
{

}


template<typename T>
std::vector<T> BVH<T>::getIntersections(const Ray& ray, std::unique_ptr<Node>& node) const
{
    if(node->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(node->mLeafValue)
        return {*(node->mLeafValue)};

    std::vector<T> leftChildren;
    std::vector<T> rightChildren;

    if(node->mLeft->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        leftChildren = getIntersections(ray, node->Left);

    if(node->mRight->mBoundingBox.intersectionDistance(ray) != NO_INTERSECTION)
        rightChildren = getIntersections(ray, node->mRight);

    return leftChildren.insert(leftChildren.back(), rightChildren.begin(), rightChildren.end());
}


template<typename T>
std::vector<std::pair<T, float>> BVH<T>::getIntersectionsWiothDistance(const Ray& ray,
                                                               std::unique_ptr<Node>& node,
                                                               const float distance) const
{
    if(node->mBoundingBox.intersectionDistance(ray) == NO_INTERSECTION)
        return {};

    if(node->mLeafValue)
        return {*(node->mLeafValue), distance};

    std::vector<std::pair<T, float>> leftChildren;
    std::vector<std::pair<T, float>> rightChildren;

    const float leftDistance = node->mLeft->mBoundingBox.intersectionDistance(ray);
    if(leftDistance != NO_INTERSECTION)
        leftChildren = getIntersectionsWithDistance(ray, node->Left, leftDistance);

    const float rightDistance = node->mRight->mBoundingBox.intersectionDistance(ray);
    if(rightDistance != NO_INTERSECTION)
        rightChildren = getIntersectionsWithDistance(ray, node->mRight);

    return leftChildren.insert(leftChildren.back(), rightChildren.begin(), rightChildren.end());
}




template<typename T>
BVH<T> BVHFactory<T>::generateBVH() const
{

}
