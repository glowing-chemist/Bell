#include "Engine/BuildAccelerationStructuresTechnique.hpp"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"


BuildAccelerationStructuresTechnique::BuildAccelerationStructuresTechnique(RenderEngine* eng, RenderGraph& graph) :
    Technique("BuildAccelerationStructures", eng->getDevice()),
    mMainAccelerationStructure(eng)
{
    Scene* scene = eng->getScene();
    for(const auto& [id, info] : scene->getInstanceMap())
    {
        if(info.mtype != InstanceType::Light)
        {
            MeshInstance *inst = scene->getMeshInstance(id);
            mMainAccelerationStructure->addInstance(inst);
        }
    }

    mMainAccelerationStructure->buildStructureOnCPU(eng);
}

void BuildAccelerationStructuresTechnique::render(RenderGraph&, RenderEngine*)
{
    mMainAccelerationStructure->updateLastAccessed(getDevice()->getCurrentSubmissionIndex());
}


void BuildAccelerationStructuresTechnique::bindResources(RenderGraph& graph)
{
    if(!graph.isResourceSlotBound(kMainCameraBVH))
    {
        graph.bindAccelerationStructure(kMainCameraBVH, mMainAccelerationStructure);
    }
}