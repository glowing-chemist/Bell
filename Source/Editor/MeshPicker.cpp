#include "MeshPicker.hpp"
#include "Engine/Camera.hpp"
#include "Engine/RayTracedScene.hpp"
#include "imgui.h"

#include <glm/glm.hpp>

void MeshPicker::tick(const Camera& cam)
{
    ImGuiIO& io = ImGui::GetIO();

    if(mRayTracedScene && io.MouseDown[0] && !io.WantCaptureMouse)
    {
        const float4x4 invView = glm::inverse(cam.getViewMatrix());
        const float4x4 invProj = glm::inverse(cam.getProjectionMatrix());

        const float4x4 invViewProj = invView * invProj;

        float4 farPosition = float4(((io.MousePos.x / io.DisplaySize.x) - 0.5f) * 2.0f,
                                    ((io.MousePos.y / io.DisplaySize.y) - 0.5f) * 2.0f, 0.9999f, 1.0f);
        farPosition = invViewProj * farPosition;
        farPosition /= farPosition.w;
        const float4 rayDir = glm::normalize(farPosition - float4(cam.getPosition(), 1.0f));
        const float3& camPositiopn = cam.getPosition();

        nanort::Ray<float> ray{};
        ray.dir[0] = rayDir.x;
        ray.dir[1] = rayDir.y;
        ray.dir[2] = rayDir.z;
        ray.org[0] = camPositiopn.x;
        ray.org[1] = camPositiopn.y;
        ray.org[2] = camPositiopn.z;
        ray.min_t = cam.getNearPlane();
        ray.max_t = cam.getFarPlane();

        int64_t instanceID;
        if (mRayTracedScene->intersectsMesh(ray, &instanceID))
        {
            mSelectedMeshInstance = instanceID;
        }
        else
            mSelectedMeshInstance = ~0ULL;
    }
}
