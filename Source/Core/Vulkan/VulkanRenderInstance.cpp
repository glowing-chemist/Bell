// Local includes
#include "VulkanRenderInstance.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"

// std library includes
#include <tuple>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>

// system includes
#include <GLFW/glfw3.h>

#define DUMP_API 0

static VKAPI_ATTR VkBool32 debugCallbackFunc(
        VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*)
{

    BELL_LOG_ARGS("VALIDATION LAYER: %s", pCallbackData->pMessage)

	BELL_TRAP;

	return false;

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


VulkanRenderInstance::VulkanRenderInstance(GLFWwindow* window) :
	RenderInstance(window)
{
    mWindow = window;

    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName("Quango");
    appInfo.setApiVersion(VK_MAKE_VERSION(1, 1, 0));
    appInfo.setPEngineName("Bell");
    appInfo.setEngineVersion(VK_MAKE_VERSION(1, 0, 0));

    uint32_t numExtensions = 0;
    const char* const* requiredExtensions = glfwGetRequiredInstanceExtensions(&numExtensions);

    std::vector<const char*> requiredExtensionVector{requiredExtensions, requiredExtensions + numExtensions};
	requiredExtensionVector.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

#if BELL_ENABLE_LOGGING
    requiredExtensionVector.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    vk::InstanceCreateInfo instanceInfo{};
    instanceInfo.setEnabledExtensionCount(requiredExtensionVector.size());
    instanceInfo.setPpEnabledExtensionNames(requiredExtensionVector.data());
    instanceInfo.setPApplicationInfo(&appInfo);
#if BELL_ENABLE_LOGGING
    const auto availableLayers = vk::enumerateInstanceLayerProperties();
    std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation",
#if DUMP_API
                                                 ,"VK_LAYER_LUNARG_api_dump"
#endif
                                                };

    std::vector<const char*> foundValidationLayers{};
    for(const auto* neededLayer : validationLayers )
    {
        for(auto& availableLayer : availableLayers) {
#ifdef __linux__
            BELL_LOG_ARGS("instance layer: %s", availableLayer.layerName.data())
#else
            BELL_LOG_ARGS("instance layer: %s", availableLayer.layerName)
#endif
            if(strcmp(availableLayer.layerName, neededLayer) == 0)
            {
                foundValidationLayers.push_back(availableLayer.layerName);
            }
        }
    }

    instanceInfo.setEnabledLayerCount(foundValidationLayers.size());
    instanceInfo.setPpEnabledLayerNames(foundValidationLayers.data());
#endif

    mInstance = vk::createInstance(instanceInfo);

#if BELL_ENABLE_LOGGING // add the debug callback as soon as possible
    addDebugCallback();
#endif


    VkSurfaceKHR surface;
    glfwCreateWindowSurface(static_cast<vk::Instance>(mInstance), mWindow, nullptr, &surface); // use glfw as is platform agnostic
    mWindowSurface = vk::SurfaceKHR(surface);
}


RenderDevice* VulkanRenderInstance::createRenderDevice(const int DeviceFeatureFlags)
{
    auto [physDev, dev] = findSuitableDevices(DeviceFeatureFlags);
    mDevice = dev;

	return new VulkanRenderDevice{mInstance, physDev, dev, mWindowSurface, mWindow};
}


std::pair<vk::PhysicalDevice, vk::Device> VulkanRenderInstance::findSuitableDevices(int DeviceFeatureFlags)
{
    const bool geometryWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Geometry;
    const bool tessWanted     = DeviceFeatureFlags & DeviceFeaturesFlags::Tessalation;
    const bool discreteWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Discrete;
    const bool computeWanted  = DeviceFeatureFlags & DeviceFeaturesFlags::Compute;
	const bool subgroupWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Subgroup;
    const bool rayTracingWanted = DeviceFeatureFlags & DeviceFeaturesFlags::RayTracing;

    auto availableDevices = mInstance.enumeratePhysicalDevices();

	int physDeviceIndex = -1;
    for(uint32_t i = 0; i < availableDevices.size(); i++) {
        const vk::PhysicalDeviceProperties properties = availableDevices[i].getProperties();
        const vk::PhysicalDeviceFeatures   features   = availableDevices[i].getFeatures();
        const QueueIndicies queueIndices = getAvailableQueues(mWindowSurface, availableDevices[i]);

#ifdef __linux__
        BELL_LOG_ARGS("Device Found: %s", properties.deviceName.data());
#else
        BELL_LOG_ARGS("Device Found: %s", properties.deviceName);
#endif
		if (geometryWanted && !features.geometryShader)
			continue;
		if (tessWanted && !features.tessellationShader)
			continue;
		if (discreteWanted && !(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu))
			continue;
		if (computeWanted && !(queueIndices.ComputeQueueIndex != -1))
			continue;
		if(subgroupWanted && !(hasSubgroupSupport(availableDevices[i])))
			continue;

		// We have a device that has all requested features.
		physDeviceIndex = i;
		break;

    }

    vk::PhysicalDevice physicalDevice = availableDevices[physDeviceIndex];

#ifdef __linux__
    BELL_LOG_ARGS("Device selected: %s", physicalDevice.getProperties().deviceName.data())
#else
    BELL_LOG_ARGS("Device selected: %s", physicalDevice.getProperties().deviceName)
#endif
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

    std::vector<const char*> optionalDeviceExtensions = {VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME};
    /*if (rayTracingWanted) // Ray tracing using compute for now.
    {
        optionalDeviceExtensions.push_back(VK_KHR_RAY_TRACING_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
    }*/


	std::vector<const char*> extensionsToEnable{};

	const auto deviceExtensionproperties = physicalDevice.enumerateDeviceExtensionProperties();

    for(const auto& availableExtensions : deviceExtensionproperties)
	{
        BELL_LOG_ARGS("Device extension Found %s", &availableExtensions.extensionName[0]);
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
    physicalFeatures.setGeometryShader(geometryWanted);
    physicalFeatures.setFragmentStoresAndAtomics(geometryWanted);
    physicalFeatures.setSamplerAnisotropy(true);
    if(rayTracingWanted)
        physicalFeatures.setShaderFloat64(true);

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
#if BELL_ENABLE_LOGGING
    const char* validationLayers = "VK_LAYER_KHRONOS_validation";
    deviceInfo.setEnabledLayerCount(1);
    deviceInfo.setPpEnabledLayerNames(&validationLayers);
#endif

    vk::Device logicalDevice = physicalDevice.createDevice(deviceInfo);

    return {physicalDevice, logicalDevice};
}

VulkanRenderInstance::~VulkanRenderInstance()
{
    mInstance.destroySurfaceKHR(mWindowSurface);
    mDevice.destroy();
#if BELL_ENABLE_LOGGING
    removeDebugCallback();
#endif
    mInstance.destroy();
    glfwDestroyWindow(mWindow);
    glfwTerminate();
}

void VulkanRenderInstance::addDebugCallback()
{
    VkDebugUtilsMessengerCreateInfoEXT callbackCreateInfo{};
    callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    callbackCreateInfo.flags = 0;
    callbackCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    callbackCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    callbackCreateInfo.pfnUserCallback = debugCallbackFunc;

    auto* createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(mInstance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    BELL_ASSERT(createMessenger, "Unable to create debug utils messenger")
    if(createMessenger != nullptr) {
        BELL_LOG("Inserting debug callback")

        auto call = static_cast<VkDebugUtilsMessengerEXT>(mDebugMessenger);
        VkResult result = createMessenger(mInstance, &callbackCreateInfo, nullptr, &call);
        BELL_ASSERT(result == VK_SUCCESS, "Failed to insert debug callback")
        mDebugMessenger = call;
    }
}


void VulkanRenderInstance::removeDebugCallback()
{
    auto* destroyMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(mInstance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
    if(destroyMessenger != nullptr)
        destroyMessenger(mInstance, mDebugMessenger, nullptr);
}


vk::SurfaceKHR VulkanRenderInstance::getSurface() const
{
    return mWindowSurface;
}


bool VulkanRenderInstance::hasSubgroupSupport(vk::PhysicalDevice physDev)
{
    vk::PhysicalDeviceProperties2 properties;
	vk::PhysicalDeviceSubgroupProperties subgroupProperties;

	properties.pNext = &subgroupProperties;

	physDev.getProperties2(&properties);

	return subgroupProperties.subgroupSize > 1;
}

