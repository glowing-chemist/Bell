#ifndef BVH_HPP
#define BVH_HPP


#include <array>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

#include "Engine/AABB.hpp"
#include "Engine/Camera.hpp"
#include "Engine/GeomUtils.h"


template<typename T>
class OctTree
{
public:
    struct Node;

    OctTree(std::unique_ptr<Node>& ptr) : mRoot{std::move(ptr)} {}
    OctTree()= default;

    OctTree(OctTree&&) = default;
    OctTree& operator=(OctTree&&) = default;

    //std::optional<T> closestIntersection(const Ray&) const;
    //std::vector<T>   allIntersections(const Ray&) const;
    std::vector<T>   containedWithin(const Frustum&, const EstimationMode) const;

    std::vector<T>	getIntersections(const AABB& aabb) const;

    struct BoundedValue
    {
        AABB mBounds;
        T mValue;
    };

    struct Node
    {
        Node() = default;

        // Use an optional incase T doesn't have a default constructor.
		AABB mBoundingBox;
        std::vector<BoundedValue> mValues;

        std::unique_ptr<Node> mChildren[8];
    };

private:
    //std::vector<T>						getIntersections(const Ray&, const std::unique_ptr<Node>&) const;
    //std::vector<std::pair<T, float>>	getIntersectionsWithDistance(const Ray&, std::unique_ptr<Node>&, const float distance) const;
		
    void	containedWithin(std::set<T>& meshes, const Frustum&, const std::unique_ptr<Node>&, const EstimationMode) const;

	void	getIntersections(const AABB& aabb, const std::unique_ptr<typename OctTree<T>::Node>& node, std::vector<T>& intersections) const;

    std::unique_ptr<Node> mRoot;
};


template<typename T>
class OctTreeFactory
{
public:

    using BuilderNode = typename OctTree<T>::BoundedValue;

    OctTreeFactory(const AABB& rootBox) : mRootBoundingBox{rootBox}, mBoundingBoxes{} {}
    OctTreeFactory(const AABB& rootBox, std::vector<typename OctTree<T>::BoundedValue>& data) : mRootBoundingBox{rootBox},
                                                                            mBoundingBoxes{data} {}
    void addAABB(const AABB& box, const T& value)
        { mBoundingBoxes.push_back({box, value}); }

    OctTree<T> generateOctTree(const uint32_t subdivisions);

private:

    std::array<AABB, 8> splitAABB(const AABB&) const;
	std::unique_ptr<typename OctTree<T>::Node> createSpacialSubdivisions(const uint32_t subdivisions, const AABB& parentBox, const std::vector<typename OctTree<T>::BoundedValue>& nodes);

    AABB mRootBoundingBox; // AABB that all others are contained within.
    std::vector<typename OctTree<T>::BoundedValue> mBoundingBoxes;
};


#endif
