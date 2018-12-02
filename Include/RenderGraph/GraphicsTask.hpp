#ifndef GRAPHICSTASK_HPP
#define GRAPHICSTASK_HPP

#include "RenderTask.hpp"

#include <string>
#include <vector>
#include <tuple>

enum class DrawType
{
    Stabdard,
    Indexed,
    Instanced,
    Indirect,
    IndexedInstanced,
    IndexedInstancedIndirect
};

enum class BlendMode
{
    None,
    Add,
    Subtract,
    Min,
    Max
};

struct Rect
{
    uint32_t x, y;
};

struct PipelineDescription
{
    std::vector<std::tuple<uint32_t, AttatchmentType>> mAttatchments;
    std::vector<std::tuple<uint32_t, AttatchmentType>> mInputs;

    std::string mVertexShaderName;
    std::string mGeometryShaderName;
    std::string mHullShaderName;
    std::string mTesselationControlShaderName;
    std::string mFragmentShaderName;

    Rect mScissorRect;
    Rect mViewport;

    BlendMode mBlendMode;
};


// This class describes a renderpass with all of it's inputs and
// outputs.
class GraphicsTask : public RenderTask
{
public:
    GraphicsTask(const std::string& name, const PipelineDescription& desc);

};

#endif
