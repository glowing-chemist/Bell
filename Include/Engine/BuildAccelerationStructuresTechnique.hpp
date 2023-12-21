#ifndef BUILD_ACCELERATION_STRUCTURES_TECHNIQUE_HPP
#define BUILD_ACCELERATION_STRUCTURES_TECHNIQUE_HPP

#include "Technique.hpp"
#include "Core/AccelerationStructures.hpp"

class BuildAccelerationStructuresTechnique : public Technique
{
public:
    BuildAccelerationStructuresTechnique(RenderEngine*, RenderGraph&);
    ~BuildAccelerationStructuresTechnique() override  = default;

    virtual PassType getPassType() const override
    {
        return PassType::BuildAccelerationStructures;
    }

    virtual void render(RenderGraph&, RenderEngine*) override;

    virtual void bindResources(RenderGraph& graph) override;

private:

    TopLevelAccelerationStructure mMainAccelerationStructure;

};

#endif