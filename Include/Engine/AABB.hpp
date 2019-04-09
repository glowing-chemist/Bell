#ifndef AABB_HPP
#define AABB_HPP

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>


// 8 float4 representing the 8 verticies of a cude
struct Cube
{
    // Verticies going clockwise looking down (perpendicular) on a single face
    glm::vec4 mUpper1;
    glm::vec4 mUpper2;
    glm::vec4 mUpper3;
    glm::vec4 mUpper4;

    glm::vec4 mLower1;
    glm::vec4 mLower2;
    glm::vec4 mLower3;
    glm::vec4 mLower4;
};


class AABB
{
public:
    AABB(const glm::vec4& diagonalTop, const glm::vec4& diagonalBottom);
    AABB(const Cube&);

    Cube getCube() const;

    AABB& operator*(const glm::mat4&);

    AABB& operator*(const glm::vec4&);
    AABB& operator+(const glm::vec4&);
    AABB& operator-(const glm::vec4&);

private:

    glm::vec4 mTopBackLeft;
    glm::vec4 mBottomFrontRight;

};


#endif
