#include "Engine/RenderQueue.hpp"
#include "Engine/Camera.hpp"
#include "Engine/Engine.hpp"


void RenderView::updateView(const Camera& cam)
{
    const Scene* scene = mEng->getScene();

    if(scene)
    {
        Frustum frustum = cam.getFrustum();
        mInstances = scene->getVisibleMeshes(frustum);

        PROFILER_EVENT("sort meshes");
        std::sort(mInstances.begin(), mInstances.end(), [camera = cam](const MeshInstance* lhs, const MeshInstance* rhs)
        {
            const float3 centralLeft = (lhs->getMesh()->getAABB() *  lhs->getTransMatrix()).getCentralPoint();
            const float leftDistance = glm::length(centralLeft - camera.getPosition());

            const float3 centralright = (rhs->getMesh()->getAABB() * rhs->getTransMatrix()).getCentralPoint();
            const float rightDistance = glm::length(centralright - camera.getPosition());
            return leftDistance < rightDistance;
        });

        mConstInstances.resize(mInstances.size());
        memcpy(mConstInstances.data(), mInstances.data(), sizeof(MeshInstance*) * mInstances.size());
    }
}