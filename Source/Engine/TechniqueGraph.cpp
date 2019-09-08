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
    // map from mesh entries to index offset and number of indicies.
    std::map<StaticMesh*, std::pair<uint32_t, uint32_t>> mMeshCache;

    // build the index/Vertex buffers (for now every frame) will probably be better to perform some
    // streaming at some point.
    // Also TODO dedupliacte meshes to use instanced draws (should be trivial).
    // And record all tasks.
    RenderGraph renderGraph;

    uint32_t currentVertexOffset = 0;
    uint32_t currentIndexOffset = 0;

    auto& vertexBuilder = mEngine->getVertexBufferBuilder();
    auto& indexBuilder = mEngine->getIndexBufferBuilder();

    for(auto& [type, technique] : mGraphicsTechniques)
    {
        for(const auto& mesh : meshes)
        {
            if(const auto it = mMeshCache.find(mesh->mMesh); it == mMeshCache.end())
            {
                mMeshCache.insert({mesh->mMesh, {currentIndexOffset, mesh->mMesh->getIndexData().size()}});

                currentVertexOffset =  static_cast<uint32_t>(vertexBuilder.addData(mesh->mMesh->getVertexData()));

                std::vector<uint32_t> offsetIndicies{};
                offsetIndicies.reserve(mesh->mMesh->getIndexData().size());

                std::transform(mesh->mMesh->getIndexData().begin(), mesh->mMesh->getIndexData().end(),
                               std::back_inserter(offsetIndicies), [vertexOffset = currentVertexOffset](const uint32_t index)
                {
                    return index + vertexOffset;
                });

                currentIndexOffset = static_cast<uint32_t>(indexBuilder.addData(offsetIndicies));
            }


            if(static_cast<uint64_t>(type) & static_cast<uint64_t>(mesh->mMesh->getPassTypes()))
            {
                const auto& cachedOffsets = mMeshCache.find(mesh->mMesh);
                technique->addMesh(mesh, cachedOffsets->second);
            }

            technique->finaliseTask();

            renderGraph.addTask(technique->getTask());
        }
    }

    for(auto& [type, technique] : mComputeTechniques)
    {
        for(const auto& mesh : meshes)
        {
            // Don't need to check the cache as all meshes will have been added by now.

            if(static_cast<uint64_t>(type) & static_cast<uint64_t>(mesh->mMesh->getPassTypes()))
            {
                const auto& cachedOffsets = mMeshCache.find(mesh->mMesh);
                technique->addMesh(mesh, cachedOffsets->second);
            }

            technique->finaliseTask();

            renderGraph.addTask(technique->getTask());
        }
    }


    // compile the dependancies if none where explicity specified (experimental).
    if(mDependancies.empty())
    {
        renderGraph.compileDependancies();
    }
    else
    {
        for(const auto& [dependancy, dependant] : mDependancies)
        {
            const std::string& dependancyTaskName = mEngine->isGraphicsTask(dependancy) ?  mGraphicsTechniques[dependancy]->getTask().getName() :
                                                                                           mComputeTechniques[dependancy]->getTask().getName();

            const std::string& dependantTaskName = mEngine->isGraphicsTask(dependant) ?  mGraphicsTechniques[dependant]->getTask().getName() :
                                                                                           mComputeTechniques[dependant]->getTask().getName();

            renderGraph.addDependancy(dependancyTaskName, dependantTaskName);
        }
    }

    // Bind all resources.
    for(const auto& [techniqueType, technique] : mGraphicsTechniques)
    {
        technique->bindResources(renderGraph);
    }
    for(const auto& [techniqueType, technique] : mComputeTechniques)
    {
        technique->bindResources(renderGraph);
    }


    return renderGraph;
}
