// Local includes
#include "RenderInstance.hpp"
#include "Core/BellLogging.hpp"

// std library includes
#include <tuple>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

// system includes
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan.h>

#define DUMP_API 0

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallbackFunc(
    VkDebugReportFlagsEXT,
    VkDebugReportObjectTypeEXT,
    uint64_t,
    size_t,
    int32_t,
    const char*,
    const char* msg,
    void*)
{

    BELL_LOG_ARGS("VALIDATION LAYER: %s", msg)

#ifdef _MSC_VER 
	__debugbreak;
#else
    asm("int3");
#endif

    return true;

}

const QueueIndicies getAvailableQueues(vk::SurfaceKHR windowSurface, vk::PhysicalDevice& dev)
{
    int graphics = -1;
    int transfer = -1;
    int compute  = -1;

    std::vector<vk::QueueFamilyProperties> queueProperties = dev.getQueueFamilyProperties();
    for(uint32_t i = 0; i < queueProperties.size(); i++) {
        if(queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) graphics = i;
        //check for compute a compute queue that can we can present on.
        if((queueProperties[i].queueFlags & vk::QueueFlagBits::eCompute) && dev.getSurfaceSupportKHR(i, windowSurface)) compute = i;
        if(queueProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) transfer = i;
        // Check for a compute only queue, this will override the previous compute queue.
        if((queueProperties[i].queueFlags & vk::QueueFlagBits::eCompute) &&
                dev.getSurfaceSupportKHR(i, windowSurface) &&
                !(queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) compute = i;
        if(graphics != -1 && transfer != -1 && compute != -1) return {graphics, transfer, compute};
    }
    return {graphics, transfer, compute};
}


RenderInstance::RenderInstance(GLFWwindow* window)
{

    mWindow = window;

    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName("Quango");
    appInfo.setApiVersion(VK_MAKE_VERSION(0, 1, 0));
    appInfo.setPEngineName("Bell");
    appInfo.setEngineVersion(VK_MAKE_VERSION(0, 1, 0));
    appInfo.setApiVersion(VK_API_VERSION_1_0);

    uint32_t numExtensions = 0;
    const char* const* requiredExtensions = glfwGetRequiredInstanceExtensions(&numExtensions);

    std::vector<const char*> requiredExtensionVector{requiredExtensions, requiredExtensions + numExtensions};
	requiredExtensionVector.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#ifndef NDEBUG
    requiredExtensionVector.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.setEnabledExtensionCount(requiredExtensionVector.size());
    instanceInfo.setPpEnabledExtensionNames(requiredExtensionVector.data());
    instanceInfo.setPApplicationInfo(&appInfo);
#ifndef NDEBUG
    const auto availableLayers = vk::enumerateInstanceLayerProperties();
    std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"
#if DUMP_API
                                                 ,"VK_LAYER_LUNARG_api_dump"
#endif
                                                };
    uint8_t layersFound = 0;
    for(const auto* neededLayer : validationLayers )
    {
        for(auto& availableLayer : availableLayers) {

            BELL_LOG_ARGS("instance layer: %s", availableLayer.layerName)

            if(strcmp(availableLayer.layerName, neededLayer) == 0) {
                ++layersFound;
            }
        }
    }
    if(layersFound != validationLayers.size()) throw std::runtime_error{"Running in debug but validation layers not found"};

    instanceInfo.setEnabledLayerCount(validationLayers.size());
    instanceInfo.setPpEnabledLayerNames(validationLayers.data());
#endif

    mInstance = vk::createInstance(instanceInfo);

#ifndef NDEBUG // add the debug callback as soon as possible
    addDebugCallback();
#endif


    VkSurfaceKHR surface;
    glfwCreateWindowSurface(static_cast<vk::Instance>(mInstance), mWindow, nullptr, &surface); // use glfw as is platform agnostic
    mWindowSurface = vk::SurfaceKHR(surface);
}


RenderDevice RenderInstance::createRenderDevice(const int DeviceFeatureFlags)
{
    auto [physDev, dev] = findSuitableDevices(DeviceFeatureFlags);
    mDevice = dev;

	return RenderDevice{mInstance, physDev, dev, mWindowSurface, mWindow};
}


std::pair<vk::PhysicalDevice, vk::Device> RenderInstance::findSuitableDevices(int DeviceFeatureFlags)
{
    bool GeometryWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Geometry;
    bool TessWanted     = DeviceFeatureFlags & DeviceFeaturesFlags::Tessalation;
    bool DiscreteWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Discrete;
    bool ComputeWanted  = DeviceFeatureFlags & DeviceFeaturesFlags::Compute;

    auto availableDevices = mInstance.enumeratePhysicalDevices();
    std::vector<int> deviceScores(availableDevices.size());

    for(uint32_t i = 0; i < availableDevices.size(); i++) {
        const vk::PhysicalDeviceProperties properties = availableDevices[i].getProperties();
        const vk::PhysicalDeviceFeatures   features   = availableDevices[i].getFeatures();
        const QueueIndicies queueIndices = getAvailableQueues(mWindowSurface, availableDevices[i]);

        BELL_LOG_ARGS("Device Found: %s", properties.deviceName)

        if(GeometryWanted && features.geometryShader) deviceScores[i] += 1;
        if(TessWanted && features.tessellationShader) deviceScores[i] += 1;
        if(DiscreteWanted && properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) deviceScores[i] += 1;
        if(ComputeWanted && (queueIndices.ComputeQueueIndex != -1)) deviceScores[i] += 1; // check if a compute queue is available
    }

    auto maxScoreeDeviceOffset = std::max_element(deviceScores.begin(), deviceScores.end());
    auto physicalDeviceIndex = std::distance(deviceScores.begin(), maxScoreeDeviceOffset);
    vk::PhysicalDevice physicalDevice = availableDevices[physicalDeviceIndex];

    BELL_LOG_ARGS("Device selected: %s", physicalDevice.getProperties().deviceName)

    const QueueIndicies queueIndices = getAvailableQueues(mWindowSurface, physicalDevice);

    float queuePriority = 1.0f;

    std::set<int> uniqueQueues{queueIndices.GraphicsQueueIndex, queueIndices.TransferQueueIndex, queueIndices.ComputeQueueIndex};
    std::vector<vk::DeviceQueueCreateInfo> queueInfo{};
    for(auto& queueIndex : uniqueQueues) {
        vk::DeviceQueueCreateInfo info{};
        info.setQueueCount(1);
        info.setQueueFamilyIndex(queueIndex);
        info.setPQueuePriorities(&queuePriority);
        queueInfo.push_back(info);
    }

	// We use descriptor indexing to implement bindless PBR so that we can do our lighting pass in one pass for all materials.
	const std::vector<const char*> requireDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
															  VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
															  VK_KHR_MAINTENANCE2_EXTENSION_NAME,
															  VK_KHR_MAINTENANCE3_EXTENSION_NAME
															 };

	const std::vector<const char*> optionalDeviceExtensions = {VK_EXT_DEBUG_MARKER_EXTENSION_NAME};

	std::vector<const char*> extensionsToEnable{};

	const auto deviceExtensionproperties = physicalDevice.enumerateDeviceExtensionProperties();

	for(const auto availableExtensions : deviceExtensionproperties)
	{
		for(const auto* extension : requireDeviceExtensions)
		{
			if(strcmp(extension, availableExtensions.extensionName) == 0)
			{
				extensionsToEnable.push_back(extension);
			}
		}

		// If the device advertises support for an optional extension add it to the required list so that we will enable it.
		for(const auto* extension : optionalDeviceExtensions)
		{
			if(strcmp(extension, availableExtensions.extensionName) == 0)
				extensionsToEnable.push_back(extension);
		}
	}

	BELL_ASSERT(extensionsToEnable.size() >= requireDeviceExtensions.size(), "Missing required extension!")

    vk::PhysicalDeviceFeatures physicalFeatures{};
    physicalFeatures.geometryShader = GeometryWanted;
    physicalFeatures.setSamplerAnisotropy(true);

	vk::PhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingInfo{};
	descriptorIndexingInfo.setShaderSampledImageArrayNonUniformIndexing(true);
	descriptorIndexingInfo.setRuntimeDescriptorArray(true);

    vk::DeviceCreateInfo deviceInfo{};
	deviceInfo.setEnabledExtensionCount(extensionsToEnable.size());
	deviceInfo.setPpEnabledExtensionNames(extensionsToEnable.data());
    deviceInfo.setQueueCreateInfoCount(uniqueQueues.size());
    deviceInfo.setPQueueCreateInfos(queueInfo.data());
    deviceInfo.setPEnabledFeatures(&physicalFeatures);
	deviceInfo.setPNext(&descriptorIndexingInfo);
#ifndef NDEBUG
    const char* validationLayers = "VK_LAYER_LUNARG_standard_validation";
    deviceInfo.setEnabledLayerCount(1);
    deviceInfo.setPpEnabledLayerNames(&validationLayers);
#endif

    vk::Device logicalDevice = physicalDevice.createDevice(deviceInfo);

    return {physicalDevice, logicalDevice};
}

RenderInstance::~RenderInstance()
{
    mInstance.destroySurfaceKHR(mWindowSurface);
    mDevice.destroy();
#ifndef NDEBUG
    removeDebugCallback();
#endif
    mInstance.destroy();
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void RenderInstance::addDebugCallback()
{
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo{};
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = debugCallbackFunc;

    auto* createCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(mInstance.getProcAddr("vkCreateDebugReportCallbackEXT"));
    if(createCallback != nullptr) {
        BELL_LOG("Inserting debug callback")

        auto call = static_cast<VkDebugReportCallbackEXT>(mDebugCallback);
        createCallback(mInstance, &callbackCreateInfo, nullptr, &call);
        mDebugCallback = call;
    }
}


void RenderInstance::removeDebugCallback()
{
    auto* destroyCallback = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(mInstance.getProcAddr("vkDestroyDebugReportCallbackEXT"));
    if(destroyCallback != nullptr)
        destroyCallback(mInstance, mDebugCallback, nullptr);
}


GLFWwindow* RenderInstance::getWindow() const
{
    return mWindow;
}


vk::SurfaceKHR RenderInstance::getSurface() const
{
    return mWindowSurface;
}


int operator|(DeviceFeaturesFlags lhs, DeviceFeaturesFlags rhs)
{
    return static_cast<int>(lhs) | static_cast<int>(rhs);
}


int operator|(int lhs, DeviceFeaturesFlags rhs)
{
	return lhs | static_cast<int>(rhs);
}


bool operator&(int flags, DeviceFeaturesFlags rhs)
{
    return flags & static_cast<int>(rhs);
}
