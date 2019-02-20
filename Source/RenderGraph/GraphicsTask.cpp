#include "RenderGraph/GraphicsTask.hpp"


void GraphicsTask::recordCommands(vk::CommandBuffer CmdBuffer)
{
    for(const auto& thunk : mDrawCalls)
    {
        switch (thunk.mDrawType)
        {
            case DrawType::Standard:
                CmdBuffer.draw(thunk.mNumberOfVerticies,
                               1,
                               thunk.mVertexOffset,
                               0);
                break;

            case DrawType::Indexed:
                CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
                                      1,
                                      thunk.mIndexOffset,
                                      static_cast<int32_t>(thunk.mVertexOffset),
                                      0);
                break;

            case DrawType::Instanced:
                CmdBuffer.draw(thunk.mNumberOfVerticies,
                               thunk.mNumberOfInstances,
                               thunk.mVertexOffset,
                               0);
                break;

            case DrawType::Indirect:
                // TODO implement indirect draw calls
                break;

            case DrawType::IndexedInstanced:
                CmdBuffer.drawIndexed(thunk.mNumberOfIndicies,
                                      thunk.mNumberOfInstances,
                                      thunk.mIndexOffset,
                                      static_cast<int32_t>(thunk.mVertexOffset),
                                      0);
                break;

            case DrawType::IndexedInstancedIndirect:
                // TODO implement indirect draw calls
                break;
        }
    }
}
