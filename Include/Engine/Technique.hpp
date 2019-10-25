#ifndef TECHNIQUE_HPP
#define TECHNIQUE_HPP


#include "Engine/PassTypes.hpp"
#include "Engine/Scene.h"
#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Core/DeviceChild.hpp"


#include <string>

class Engine;

class Technique : public DeviceChild
{
public:
    Technique(const std::string& name, RenderDevice* dev) :
		DeviceChild{dev},
        mName{name} {}

    virtual ~Technique() = default;

    virtual PassType getPassType() const = 0;

    // default empty implementations as most classes won't need to do anything for one of these.
	virtual void render(RenderGraph&, Engine*, const std::vector<const Scene::MeshInstance*>&) = 0;

    virtual void bindResources(RenderGraph&) const = 0;

private:

    std::string mName;
};

#endif
