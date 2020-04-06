#include "DX_12RenderDevice.hpp"

DX_12RenderDevice::DX_12RenderDevice(ID3D12Device6* dev, GLFWwindow* window) :
	mDevice(dev),
	mWindow(window)
{

}


DX_12RenderDevice::~DX_12RenderDevice()
{

}