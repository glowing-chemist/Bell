#include "SwapChain.hpp"
#include "RenderInstance.hpp"
#include "RenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/ConversionUtils.hpp"

#include <GLFW/glfw3.h>


uint32_t SwapChainBase::getSwapChainImageWidth() const
{
    return mSwapChainExtent.width;
}


uint32_t SwapChainBase::getSwapChainImageHeight() const
{
    return mSwapChainExtent.height;
}


uint32_t SwapChainBase::getNumberOfSwapChainImages() const
{
    return mSwapChainImages.size();
}


ImageExtent SwapChainBase::chooseSwapExtent(GLFWwindow* window)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    return ImageExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

SwapChainBase::SwapChainBase(RenderDevice* Device, GLFWwindow* window) :
    DeviceChild{Device},
    mCurrentImageIndex{0},
	mWindow{window}
{
}

