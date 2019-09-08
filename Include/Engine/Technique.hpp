#ifndef TECHNIQUE_HPP
#define TECHNIQUE_HPP


#include "Engine/PassTypes.hpp"
#include "Engine/Scene.h"
#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Core/DeviceChild.hpp"


#include <string>


template<typename T>
class Technique : public DeviceChild
{
public:
    Technique(const std::string& name, RenderDevice* dev) :
		DeviceChild{dev},
        mName{name} {}

    virtual ~Technique() = default;

    virtual PassType getPassType() const = 0;

    // default empty implementations as most classes won't need to do anything for one of these.
    virtual void addMesh(const Scene::MeshInstance*, const std::pair<uint32_t, uint32_t>&) {}
    virtual void finaliseTask() {}

    virtual void bindResources(RenderGraph&) const = 0;

    virtual T& getTask() = 0;
private:

    std::string mName;
};

#endif
