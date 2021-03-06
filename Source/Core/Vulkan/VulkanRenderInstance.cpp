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

// Storage for the dynamic dispatcher.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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

QueueIndicies getAvailableQueues(vk::SurfaceKHR windowSurface, vk::PhysicalDevice& dev)
{
    int graphics = -1;
    int transfer = -1;
    int compute  = -1;

    std::vector<vk::QueueFamilyProperties> queueProperties = dev.getQueueFamilyProperties();
    for(uint32_t i = 0; i < queueProperties.size(); i++) 
    {
        // Check for graphics queue to present from.
        if((queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && dev.getSurfaceSupportKHR(i, windowSurface))
            graphics = i;

        //check for compute a dedicated compute queue
        if((queueProperties[i].queueFlags & vk::QueueFlagBits::eCompute) && !(queueProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)) 
            compute = i;

        // Just pick any transfer queue for now.
        if(queueProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) transfer = i;

        if(graphics != -1 && transfer != -1 && compute != -1) 
            return {graphics, transfer, compute};
    }

    BELL_ASSERT(graphics != -1, "Unable to find valid graphics queue")
    if (compute == -1)
        compute = graphics;
    return {graphics, transfer, compute};
}


VulkanRenderInstance::VulkanRenderInstance(GLFWwindow* window) :
	RenderInstance(window)
{
    mWindow = window;

    // Init the dynamic dispatcher.
    {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = mLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    }

    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName("Quango");
    appInfo.setApiVersion(VK_MAKE_VERSION(1, 2, 0));
    appInfo.setPEngineName("Bell");
    appInfo.setEngineVersion(VK_MAKE_VERSION(1, 1, 0));

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
    VULKAN_HPP_DEFAULT_DISPATCHER.init(mInstance);

#if BELL_ENABLE_LOGGING // add the debug callback as soon as possible
    addDebugCallback();
#endif

    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(mInstance, mWindow, nullptr, &surface); // use glfw as is platform agnostic
    BELL_ASSERT(result == VK_SUCCESS, "Failed to create window surface")
    mWindowSurface = vk::SurfaceKHR(surface);
}


RenderDevice* VulkanRenderInstance::createRenderDevice(int DeviceFeatureFlags, const bool vsync)
{
    auto [physDev, dev] = findSuitableDevices(DeviceFeatureFlags);
    mDevice = dev;
    VULKAN_HPP_DEFAULT_DISPATCHER.init(mDevice);

	return new VulkanRenderDevice{mInstance, physDev, dev, mWindowSurface, mLoader, mWindow, static_cast<uint32_t>(DeviceFeatureFlags), vsync};
}


std::pair<vk::PhysicalDevice, vk::Device> VulkanRenderInstance::findSuitableDevices(int& DeviceFeatureFlags)
{
    const bool geometryWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Geometry;
    const bool tessWanted     = DeviceFeatureFlags & DeviceFeaturesFlags::Tessalation;
    const bool discreteWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Discrete;
    const bool computeWanted  = DeviceFeatureFlags & DeviceFeaturesFlags::Compute;
	const bool subgroupWanted = DeviceFeatureFlags & DeviceFeaturesFlags::Subgroup;
    const bool rayTracingWanted = DeviceFeatureFlags & DeviceFeaturesFlags::RayTracing;

    auto availableDevices = mInstance.enumeratePhysicalDevices();

	int physDeviceIndex = 1;
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
    for(auto& queueIndex : uniqueQueues)
    {
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
															  VK_KHR_MAINTENANCE3_EXTENSION_NAME,
                                                              VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
															 };

    std::vector<const char*> optionalDeviceExtensions = {VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME};
    if (rayTracingWanted) // Ray tracing using compute for now.
    {
        // extensions needed for ray tracing.
        optionalDeviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        optionalDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
    }


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

	if(std::find_if(extensionsToEnable.begin(), extensionsToEnable.end(), [](auto* extension) {return strcmp(extension, VK_KHR_RAY_QUERY_EXTENSION_NAME) == 0;}) == extensionsToEnable.end())
        DeviceFeatureFlags &= ~static_cast<int>(DeviceFeaturesFlags::RayTracing);

    vk::PhysicalDeviceFeatures physicalFeatures{};
    physicalFeatures.setGeometryShader(geometryWanted);
    physicalFeatures.setFragmentStoresAndAtomics(geometryWanted);
    physicalFeatures.setSamplerAnisotropy(true);
    physicalFeatures.setShaderImageGatherExtended(true);
    physicalFeatures.setFillModeNonSolid(true);
    physicalFeatures.setFragmentStoresAndAtomics(true);

	vk::PhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingInfo{};
	descriptorIndexingInfo.setShaderSampledImageArrayNonUniformIndexing(true);
	descriptorIndexingInfo.setRuntimeDescriptorArray(true);

    vk::PhysicalDeviceTimelineSemaphoreFeatures timeLineSepahmoreFeature{};
    timeLineSepahmoreFeature.setTimelineSemaphore(true);

    vk::PhysicalDeviceRayQueryFeaturesKHR rayQueryFeature{};
    rayQueryFeature.rayQuery = rayTracingWanted;
    timeLineSepahmoreFeature.setPNext(&rayQueryFeature);

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeature{};
    accelerationStructureFeature.accelerationStructure = rayTracingWanted;
    accelerationStructureFeature.accelerationStructureHostCommands = rayTracingWanted;
    rayQueryFeature.setPNext(&accelerationStructureFeature);

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayPipelineFeature{};
    rayPipelineFeature.rayTracingPipeline = rayTracingWanted;
    rayPipelineFeature.rayTraversalPrimitiveCulling = rayTracingWanted;
    accelerationStructureFeature.setPNext(&rayPipelineFeature);

    descriptorIndexingInfo.setPNext(&timeLineSepahmoreFeature);

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
    callbackCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
    callbackCreateInfo.pfnUserCallback = debugCallbackFunc;

    auto* createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(mInstance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
    BELL_ASSERT(createMessenger, "Unable to create debug utils messenger")
    if(createMessenger != nullptr)
    {
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

