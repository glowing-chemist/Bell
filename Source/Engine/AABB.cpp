#include "Engine/AABB.hpp"


AABB::AABB(const glm::vec4& diagonalTop, const glm::vec4& diagonalBottom) :
    mTopBackLeft{diagonalTop},
    mBottomFrontRight{diagonalBottom}
{}


AABB::AABB(const Cube& cube) :
    mTopBackLeft{cube.mUpper1},
    mBottomFrontRight{cube.mLower3}
{}


Cube AABB::getCube() const
{
    glm::vec4 upper1{mTopBackLeft};
    glm::vec4 upper2{mBottomFrontRight.x, mTopBackLeft.y, mTopBackLeft.z, mTopBackLeft.w};
    glm::vec4 upper3{mBottomFrontRight.x, mTopBackLeft.y, mBottomFrontRight.z, mTopBackLeft.w};
    glm::vec4 upper4{mTopBackLeft.x, mTopBackLeft.y, mBottomFrontRight.z, mTopBackLeft.w};

    glm::vec4 lower1{mTopBackLeft.x, mBottomFrontRight.y, mTopBackLeft.z, mBottomFrontRight.w};
    glm::vec4 lower2{mBottomFrontRight.x, mBottomFrontRight.y, mTopBackLeft.z, mBottomFrontRight.w};
    glm::vec4 lower3{mBottomFrontRight};
    glm::vec4 lower4{mTopBackLeft.x, mBottomFrontRight.y, mBottomFrontRight.z, mBottomFrontRight.w};

    return Cube{upper1, upper2, upper3, upper4,
                lower1, lower2, lower3, lower4};
}


AABB& AABB::operator*(const glm::mat4& mat)
{
    mTopBackLeft = mat * mTopBackLeft;
    mBottomFrontRight = mat * mBottomFrontRight;

    return *this;
}


AABB& AABB::operator*(const glm::vec4& vec)
{
    mTopBackLeft *= vec;
    mBottomFrontRight *= vec;

    return *this;
}


AABB& AABB::operator+(const glm::vec4& vec)
{
    mTopBackLeft += vec;
    mBottomFrontRight += vec;

    return *this;
}


AABB& AABB::operator-(const glm::vec4& vec)
{
    mTopBackLeft -= vec;
    mBottomFrontRight -= vec;

    return *this;
}
