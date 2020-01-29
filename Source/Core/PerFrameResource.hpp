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

    template<typename ...Args>
    PerFrameResource(RenderDevice* dev, Args&& ... args) :
        DeviceChild{dev},
        mData{}
    {
        mData.reserve(getDevice()->getSwapChainImageCount());
        for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
        {
            mData.emplace_back(dev, std::forward<Args>(args)...);
        }
    }

    template<typename ...Args, typename K>
    PerFrameResource(PerFrameResource<K>& parent, Args&& ... args) :
        DeviceChild{parent.getDevice()},
        mData{}
    {
        mData.reserve(getDevice()->getSwapChainImageCount());
        for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
        {
            mData.emplace_back(parent.get(i), std::forward<Args>(args)...);
        }
    }

    /* TODO Make this work for non DeviceChild derived classes.
    template<typename E>
    PerFrameResource(E* eng) :
        DeviceChild{eng->getDevice()},
        mData{}
    {
        for(uint32_t i = 0; i < getDevice()->getSwapChainImageCount(); ++i)
        {
            mData.emplace_back(eng);
        }
    }

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

    T& get(const uint32_t index)
    { return mData[index]; }

    const T& get(const uint32_t index) const
    {
        return mData[index];
    }

	T& get()
	{
		return mData[getDevice()->getCurrentFrameIndex()];
	}

	const T& get() const
	{
		return mData[getDevice()->getCurrentFrameIndex()];
	}

private:

    std::vector<T> mData;
};


#endif
