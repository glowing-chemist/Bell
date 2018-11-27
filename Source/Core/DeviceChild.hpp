#ifndef DEVICECHILD_HPP
#define DEVICECHILD_HPP

class RenderDevice;

class DeviceChild {
public:
    DeviceChild(RenderDevice* dev) : mDevice{dev} {}

    RenderDevice* getDevice() { return mDevice; }

private:
   RenderDevice*  mDevice;
};


#endif
