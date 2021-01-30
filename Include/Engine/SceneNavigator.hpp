#ifndef SCENE_NAVIGATOR_HPP
#define SCENE_NAVIGATOR_HPP

#include "Recast.h"
#include "DetourNavMesh.h"

#include "Core/BellLogging.hpp"


class Scene;

class SceneNavigator
{
public:
    SceneNavigator(Scene*);
    ~SceneNavigator();

    void regenrateNavMesh()
    {
        const bool success = generateNavmesh();
        BELL_ASSERT(success, "Failed to build navmesh")
    }




private:

    bool generateNavmesh();


    Scene* mScene;

    rcHeightfield* m_solid;
    rcCompactHeightfield* m_chf;
    rcContourSet* m_cset;
    rcPolyMesh* m_pmesh;
    rcPolyMeshDetail* m_dmesh;
    dtNavMesh* m_navMesh;
};


#endif
