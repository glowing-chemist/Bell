#ifndef TECHNIQUEGRAPH_HPP
#define TECHNIQUEGRAPH_HPP

#include "Core/PerFrameResource.hpp"

#include "RenderGraph/RenderTask.hpp"

#include "PassTypes.hpp"
#include "Technique.hpp"
#include "Scene.h"

#include <map>
#include <tuple>

class Engine;


class TechniqueGraph
{
public:
    TechniqueGraph(Engine*);
    ~TechniqueGraph() = default;


    void addPass(const PassType);
    void addDependancy(const PassType, const PassType);

    RenderGraph generateRenderGraph(const std::vector<Scene::MeshInstance*>&);


private:

    Engine* mEngine;

    // For now only handle one of each pass.
    // TODO extent using a multimap.
    std::map<PassType, std::unique_ptr<Technique<GraphicsTask>>> mGraphicsTechniques;
    std::map<PassType, std::unique_ptr<Technique<ComputeTask>>>  mComputeTechniques;

    // Dependancy, dependant.
    std::vector<std::pair<PassType, PassType>> mDependancies;
};

#endif
