#ifndef RENDERTASK_HPP
#define RENDERTASK_HPP

// This class describes any task that can be used for rendering
// from graphics render passes to async compute

enum class AttatchmentType
{
    Texture1D,
    Texture2D,
    Texture3D,
    UniformBuffer,
    DataBuffer,
    PushConstants
};


class RenderTask {


};

#endif
