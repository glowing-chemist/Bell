#ifndef TECHNIQUE_HPP
#define TECHNIQUE_HPP


#include "Engine/PassTypes.hpp"
#include "Engine/Scene.h"
#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Core/DeviceChild.hpp"


#include <string>

class RenderEngine;

class Technique : public DeviceChild
{
public:
    Technique(const std::string& name, RenderDevice* dev) :
		DeviceChild{dev},
        mName{name} {}

    virtual ~Technique() = default;

    virtual PassType getPassType() const = 0;

	virtual void render(RenderGraph&, RenderEngine*) = 0;

    virtual void bindResources(RenderGraph&) = 0;

    virtual void postGraphCompilation(RenderGraph&, RenderEngine*) {}

    const std::string getName() const
    {
        return mName;
    }

private:

    std::string mName;
};

#endif
