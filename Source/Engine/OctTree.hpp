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


constexpr uint32_t kInvalidNodeIndex = ~0u;
using NodeIndex = uint32_t;


template<typename T>
class OctTree
{
public:
    struct Node;

    OctTree(const NodeIndex rootIndex, std::vector<Node>& nodeStorage) : mTests{0},  mRoot{rootIndex}, mNodes{nodeStorage} {}
    OctTree() : mTests{0}, mRoot{kInvalidNodeIndex} , mNodes{} {}

    OctTree(OctTree&&) = default;
    OctTree& operator=(OctTree&&) = default;

    //std::optional<T> closestIntersection(const Ray&) const;
    //std::vector<T>   allIntersections(const Ray&) const;
    std::vector<T>   containedWithin(const Frustum&) const;

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

       uint32_t mChildCount;
       NodeIndex mChildren[8];
    };

    uint32_t getTestsPerformed() const
    {
	return mTests;
    }

private:
    //std::vector<T>						getIntersections(const Ray&, const std::unique_ptr<Node>&) const;
    //std::vector<std::pair<T, float>>	getIntersectionsWithDistance(const Ray&, std::unique_ptr<Node>&, const float distance) const;
		
    const Node& getNode(const NodeIndex n) const
    {
        BELL_ASSERT(n != kInvalidNodeIndex && n < mNodes.size(), "Invalid index")
        return mNodes[n];
    }

    void	containedWithin(std::vector<T> &meshes, const Frustum&, const Node &, const Intersection nodeFlags) const;

    void	getIntersections(const AABB& aabb, const typename OctTree<T>::Node& node, std::vector<T>& intersections, const Intersection nodeFlags) const;

    mutable uint32_t mTests;
    NodeIndex mRoot;

    std::vector<Node> mNodes;
};


template<typename T>
class OctTreeFactory
{
public:

    using BuilderNode = typename OctTree<T>::BoundedValue;

    OctTreeFactory(const AABB& rootBox, std::vector<typename OctTree<T>::BoundedValue>& data) : mRootBoundingBox{rootBox},
                                                                            mBoundingBoxes{data} {}


    OctTree<T> generateOctTree();

private:

    NodeIndex addNode(const typename OctTree<T>::Node& n)
    {
        const NodeIndex i = mNodeStorage.size();
        mNodeStorage.push_back(n);

        return i;
    }

    std::array<AABB, 8> splitAABB(const AABB&) const;
    NodeIndex createSpacialSubdivisions(const AABB& parentBox,
                                        const std::vector<typename OctTree<T>::BoundedValue>& nodes);

    std::vector<typename OctTree<T>::Node> mNodeStorage;

    AABB mRootBoundingBox; // AABB that all others are contained within.
    std::vector<typename OctTree<T>::BoundedValue> mBoundingBoxes;
};


#endif
