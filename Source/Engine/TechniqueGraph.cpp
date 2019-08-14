#include "Engine/TechniqueGraph.hpp"

#include "Engine/Engine.hpp"

TechniqueGraph::TechniqueGraph(Engine* eng) :
    mEngine{eng}
{
}


void TechniqueGraph::addPass(const PassType passType)
{

}


void TechniqueGraph::addDependancy(const PassType dependency, const PassType dependant)
{
    mDependancies.push_back({dependency, dependant});
}


RenderGraph TechniqueGraph::generateRenderGraph(const std::vector<Scene::MeshInstance*>& meshes)
{

}
