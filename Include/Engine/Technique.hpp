#ifndef TECHNIQUE_HPP
#define TECHNIQUE_HPP


#include "Engine/PassTypes.hpp"
#include "RenderGraph/RenderTask.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Core/DeviceChild.hpp"


#include <string>


class Technique : public DeviceChild
{
public:
	Technique(const std::string& name, RenderDevice* dev) :
		DeviceChild{dev},
		mName{name} {}

    virtual ~Technique() = default;

    virtual PassType getPassType() const = 0;

	virtual RenderTask& getTaskToRecord() = 0;

private:

    std::string mName;

};


#endif
