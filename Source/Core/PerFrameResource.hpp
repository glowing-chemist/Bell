#ifndef PERFRAMERESOURCE_HPP
#define PERFRAMERESOURCE_HPP


#include "Core/DeviceChild.hpp"
#include "Core/RenderDevice.hpp"

#include <type_traits>
#include <vector>


template<typename T>
class PerFrameResource : public DeviceChild
{
public:

    template<typename ...Args, typename D = std::enable_if_t<std::is_base_of<DeviceChild, T>::value, RenderDevice>>
    PerFrameResource(RenderDevice* dev, Args&& ... args) :
        DeviceChild{dev},
        mData{}
    {
        for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
        {
            mData.emplace_back(dev, std::forward<Args>(args)...);
        }
    }

    /* TODO Make this work for non DeviceChild derived classes.
    template<typename ...Args, typename D = std::enable_if_t<std::negation<std::is_base_of<DeviceChild, T>>::value, RenderDevice>>
    PerFrameResource(D* dev, Args&& ... args) :
        DeviceChild{dev},
        mData{}
    {
        for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
        {
            mData.emplace_back(std::forward<Args>(args)...);
        }
    }
    */

    T* operator->()
    { return &mData[getDevice()->getCurrentFrameIndex()]; }

    const T* operator->() const
    { return &mData[getDevice()->getCurrentFrameIndex()]; }

    const T& operator*() const
     { return mData[getDevice()->getCurrentFrameIndex()]; }

    T& operator*()
     { return mData[getDevice()->getCurrentFrameIndex()]; }

private:

    std::vector<T> mData;
};


#endif
