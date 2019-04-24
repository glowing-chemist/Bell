#ifndef SCENE_HPP
#define SCENE_HPP

#include "Engine/BVH.hpp"
#include "Engine/Camera.hpp"
#include "Engine/StaticMesh.h"

#include <cstdint>
#include <string>
#include <vector>


using SceneID = int64_t;

enum class MeshType
{
	Dynamic,
	Static
};


class Scene
{
public:
	Scene(const std::string& name);

	SceneID addMesh(const StaticMesh&, MeshType);
	
	void	setCamera(const Camera& camera)
	{
		mSceneCamera = camera;
	}
	
	Camera& getCamera()
	{
		return mSceneCamera;
	}

private:

	std::vector<StaticMesh> mStaticMeshes;
	std::vector<StaticMesh> mDynamicMeshes;

	Camera mSceneCamera;

};

#endif
