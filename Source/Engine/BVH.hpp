#ifndef BVH_HPP
#define BVH_HPP


#include <memory>
#include <optional>
#include <tuple>
#include <vector>

#include "Engine/AABB.hpp"
#include "Engine/Camera.hpp"
#include "Engine/GeomUtils.h"


template<typename T>
class BVH
{
public:
    struct Node;
    BVH(std::unique_ptr<Node>& ptr) : mRoot{std::move(ptr)} {}

    std::optional<T> closestIntersection(const Ray&) const;
    std::vector<T>   allIntersections(const Ray&) const;
    std::vector<T>   containedWithin(const Frustum&, const EstimationMode) const;

    struct Node
    {
        // Use an optional incase T doesn't have a default constructor.
        std::optional<T> mLeafValue;

        std::unique_ptr<Node> mLeft;
        std::unique_ptr<Node> mRight;

        AABB mBoundingBox;
    };

private:
    std::vector<T>              getIntersections(const Ray&, const std::unique_ptr<Node>&) const;
    std::vector<std::pair<T, float>>   getIntersectionsWithDistance(const Ray&, std::unique_ptr<Node>&, const float distance) const;

	std::vector<T>              containedWithin(const Frustum&, const std::unique_ptr<Node>&, const EstimationMode) const;

    std::unique_ptr<Node> mRoot;
};


template<typename T>
class BVHFactory
{
public:
    BVHFactory(const AABB& rootBox) : mRootBoundingBox{rootBox}, mBoundingBoxes{} {}

    void addAABB(const AABB& box, const T& value)
        { mBoundingBoxes.push_back({box, value}); }

    BVH<T> generateBVH() const;

private:
    AABB mRootBoundingBox; // AABB that all others are contained within.
    std::vector<std::pair<AABB, T>> mBoundingBoxes;
};


#endif
