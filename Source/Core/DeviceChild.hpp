#ifndef DEVICECHILD_HPP
#define DEVICECHILD_HPP

#include <memory>
#include <optional>
#include <vector>


class RenderDevice;


class DeviceChild {
public:
    DeviceChild(RenderDevice* dev) : mDevice{dev} {}

    RenderDevice* getDevice() { return mDevice; }

    const RenderDevice* getDevice() const { return mDevice; }

private:
   RenderDevice*  mDevice;
};


#endif
