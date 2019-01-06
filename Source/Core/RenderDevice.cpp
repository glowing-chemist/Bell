#include "RenderDevice.hpp"
#include "RenderInstance.hpp"
#include <vulkan/vulkan.hpp>


RenderDevice::RenderDevice(vk::PhysicalDevice physDev, vk::Device dev, vk::SurfaceKHR surface, GLFWwindow* window) :
    mCurrentSubmission{0},
    mFinishedSubmission{0},
    mDevice{dev},
    mPhysicalDevice{physDev},
    mSwapChain{mDevice, mPhysicalDevice, surface, window},
    mMemoryManager{this} 
{

    mQueueFamilyIndicies = getAvailableQueues(surface, mPhysicalDevice);
    mGraphicsQueue = mDevice.getQueue(mQueueFamilyIndicies.GraphicsQueueIndex, 0);
    mComputeQueue  = mDevice.getQueue(mQueueFamilyIndicies.ComputeQueueIndex, 0);
    mTransferQueue = mDevice.getQueue(mQueueFamilyIndicies.TransferQueueIndex, 0);

	// Create a command pool for each frame.
	for (uint32_t i = 0; i < mSwapChain.getNumberOfSwapChainImages(); ++i)
	{
		mCommandPools.push_back(CommandPool{ this });
	}
}

vk::Image   RenderDevice::createImage(const vk::Format format,
                                      const vk::ImageUsageFlags usage,
                                      const vk::ImageType type,
                                      const uint32_t x,
                                      const uint32_t y,
                                      const uint32_t z)
{
    // default to only allowing 2D images for now, with no MS
    vk::ImageCreateInfo createInfo(vk::ImageCreateFlags{},
                                  type,
                                  format,
                                  vk::Extent3D{x, y, z},
                                  0,
                                  1,
                                  vk::SampleCountFlagBits::e1,
                                  vk::ImageTiling::eOptimal,
                                  usage,
                                  vk::SharingMode::eExclusive,
                                  0,
                                  nullptr,
                                  vk::ImageLayout::eUndefined);

    return mDevice.createImage(createInfo);
}


vk::Buffer  RenderDevice::createBuffer(const uint32_t size, const vk::BufferUsageFlags usage)
{
    vk::BufferCreateInfo createInfo{};
    createInfo.setSize(size);
    createInfo.setUsage(usage);

    return mDevice.createBuffer(createInfo);
}


vk::Queue&  RenderDevice::getQueue(const QueueType type)
{
    switch(type)
    {
        case QueueType::Graphics:
            return mGraphicsQueue;
        case QueueType::Compute:
            return mComputeQueue;
        case QueueType::Transfer:
            return mTransferQueue;
    }
    return mGraphicsQueue; // This should be unreachable unless I add more queue types.
}


uint32_t RenderDevice::getQueueFamilyIndex(const QueueType type) const
{
	switch(type)
	{
	case QueueType::Graphics:
		return mQueueFamilyIndicies.GraphicsQueueIndex;
	case QueueType::Compute:
		return mQueueFamilyIndicies.ComputeQueueIndex;
	case QueueType::Transfer:
		return mQueueFamilyIndicies.TransferQueueIndex;
	}
	return mQueueFamilyIndicies.GraphicsQueueIndex; // This should be unreachable unless I add more queue types.
}


std::pair<vk::Pipeline, vk::PipelineLayout>	RenderDevice::generatePipelineFromTask(GraphicsTask& task)
{
    return {nullptr, nullptr};
}


std::pair<vk::Pipeline, vk::PipelineLayout>	RenderDevice::generatePipelineFromTask(ComputeTask& task)
{
    return {nullptr, nullptr};
}


vk::RenderPass	RenderDevice::generateRenderPassFromTask(GraphicsTask& task)
{
    // TODO implement a renderpass cache.

    const auto& inputAttachments = task.getInputAttachments();
    const auto& outputAttachments = task.getOuputAttachments();

    std::vector<vk::AttachmentDescription> attachmentDescriptions;

    for(const auto& [name, type] : inputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBuffer ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer)
                continue;

        vk::AttachmentDescription attachmentDesc{};

        // get the needed format
        const std::pair<vk::Format, vk::ImageLayout> formatandLayout = [type, this]()
        {
            switch(type)
            {
                case AttachmentType::Texture1D:
                case AttachmentType::Texture2D:
                case AttachmentType::Texture3D:
                    return std::make_pair(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal);
                case AttachmentType::Depth:
                    return std::make_pair(vk::Format::eD32Sfloat, vk::ImageLayout::eDepthStencilReadOnlyOptimal);
                case AttachmentType::SwapChain:
                    return std::make_pair(this->getSwapChain()->getSwapChainImageFormat(),
                                          vk::ImageLayout::eShaderReadOnlyOptimal);
                default:
                    return std::make_pair(vk::Format::eR8Sint, vk::ImageLayout::eUndefined); // should be obvious that something has gone wrong.
            }
        }();

        attachmentDesc.setFormat(formatandLayout.first);

        // eventually I will implment a render pass system that is aware of what comes beofer and after it
        // in order to avoid having to do manula barriers for all transitions.
        attachmentDesc.setInitialLayout((formatandLayout.second));
        attachmentDesc.setFinalLayout(formatandLayout.second);

        attachmentDesc.setLoadOp(vk::AttachmentLoadOp::eLoad); // we are going to overwrite all pixles
        attachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachmentDesc.setStencilLoadOp(vk::AttachmentLoadOp::eLoad);
        attachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eStore);

        attachmentDescriptions.push_back((attachmentDesc));
    }

    for(const auto& [name, type] : outputAttachments)
    {
        // We only care about images here.
        if(type == AttachmentType::DataBuffer ||
           type == AttachmentType::PushConstants ||
           type == AttachmentType::UniformBuffer)
                continue;

        vk::AttachmentDescription attachmentDesc{};

        // get the needed format
        const std::pair<vk::Format, vk::ImageLayout> formatandLayout = [type, this]()
        {
            switch(type)
            {
                case AttachmentType::RenderTarget1D:
                case AttachmentType::RenderTarget2D:
                case AttachmentType::RenderTarget3D:
                    return std::make_pair(vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eColorAttachmentOptimal);
                case AttachmentType::Depth:
                    return std::make_pair(vk::Format::eD32Sfloat, vk::ImageLayout::eDepthStencilAttachmentOptimal);
                case AttachmentType::SwapChain:
                    return std::make_pair(this->getSwapChain()->getSwapChainImageFormat(),
                                          vk::ImageLayout::eColorAttachmentOptimal);
                default:
                    return std::make_pair(vk::Format::eR8Sint, vk::ImageLayout::eUndefined); // should be obvious that something has gone wrong.
            }
        }();

        attachmentDesc.setFormat(formatandLayout.first);

        // eventually I will implment a render pass system that is aware of what comes beofer and after it
        // in order to avoid having to do manula barriers for all transitions.
        attachmentDesc.setInitialLayout((formatandLayout.second));
        attachmentDesc.setFinalLayout(formatandLayout.second);

        attachmentDesc.setLoadOp(vk::AttachmentLoadOp::eLoad); // we are going to overwrite all pixles
        attachmentDesc.setStoreOp(vk::AttachmentStoreOp::eStore);
        attachmentDesc.setStencilLoadOp(vk::AttachmentLoadOp::eLoad);
        attachmentDesc.setStencilStoreOp(vk::AttachmentStoreOp::eStore);

        attachmentDescriptions.push_back((attachmentDesc));
    }

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.setAttachmentCount(attachmentDescriptions.size());
    renderPassInfo.setPAttachments(attachmentDescriptions.data());

    return mDevice.createRenderPass(renderPassInfo);
}


std::pair<vk::VertexInputBindingDescription, std::vector<vk::VertexInputAttributeDescription>> generateVertexInputFromTask(const GraphicsTask& task)
{
    const int vertexInputs = task.getVertexAttributes();

    const bool hasPosition = vertexInputs & VertexAttributes::Position;
    const bool hasTextureCoords = vertexInputs & VertexAttributes::TextureCoordinates;
    const bool hasNormals = vertexInputs & VertexAttributes::Normals;
    const bool hasAlbedo = vertexInputs & VertexAttributes::Aledo;

    const uint32_t vertexStride = (hasPosition ? 4 : 0) + (hasTextureCoords ? 2 : 0) + (hasNormals ? 4 : 0) + (hasAlbedo ? 1 : 0);

    vk::VertexInputBindingDescription bindingDesc{};
    bindingDesc.setStride(vertexStride);
    bindingDesc.setBinding(0);
    bindingDesc.setInputRate(vk::VertexInputRate::eVertex); // only support the same model for instances draws currently.

    std::vector<vk::VertexInputAttributeDescription> attribs;
    uint8_t currentOffset = 0;
    uint8_t currentLocation  = 0;

    if(hasPosition)
    {
        vk::VertexInputAttributeDescription attribDescPos{};
        attribDescPos.setBinding(0);
        attribDescPos.setLocation(currentLocation);
        attribDescPos.setFormat(vk::Format::eR32G32B32A32Sfloat);
        attribDescPos.setOffset(currentOffset);

        attribs.push_back(attribDescPos);
        currentOffset += 16;
        ++currentLocation;
    }

    if(hasTextureCoords)
    {
        vk::VertexInputAttributeDescription attribDescTex{};
        attribDescTex.setBinding(0);
        attribDescTex.setLocation(currentLocation);
        attribDescTex.setFormat(vk::Format::eR32G32Sfloat);
        attribDescTex.setOffset(currentOffset);

        attribs.push_back(attribDescTex);
        currentOffset += 8;
        ++currentLocation;
    }

    if(hasNormals)
    {
        vk::VertexInputAttributeDescription attribDescNormal{};
        attribDescNormal.setBinding(0);
        attribDescNormal.setLocation(currentLocation);
        attribDescNormal.setFormat(vk::Format::eR32G32B32A32Sfloat);
        attribDescNormal.setOffset(currentOffset);

        attribs.push_back(attribDescNormal);
        currentOffset += 16;
        ++currentLocation;
    }

    if(hasAlbedo)
    {
        vk::VertexInputAttributeDescription attribDescAlbedo{};
        attribDescAlbedo.setBinding(0);
        attribDescAlbedo.setLocation(currentLocation);
        attribDescAlbedo.setFormat(vk::Format::eR32Sfloat);
        attribDescAlbedo.setOffset(currentOffset);

        attribs.push_back(attribDescAlbedo);
    }

    return {bindingDesc, attribs};
}


// Memory management functions
vk::PhysicalDeviceMemoryProperties RenderDevice::getMemoryProperties() const
{
    return mPhysicalDevice.getMemoryProperties();
}


vk::DeviceMemory    RenderDevice::allocateMemory(vk::MemoryAllocateInfo allocInfo)
{
    return mDevice.allocateMemory(allocInfo);
}


void    RenderDevice::freeMemory(const vk::DeviceMemory memory)
{
    mDevice.freeMemory(memory);
}


void*   RenderDevice::mapMemory(vk::DeviceMemory memory, vk::DeviceSize size, vk::DeviceSize offset)
{
    return mDevice.mapMemory(memory, offset, size);
}


void    RenderDevice::unmapMemory(vk::DeviceMemory memory)
{
    mDevice.unmapMemory(memory);
}


void    RenderDevice::bindBufferMemory(vk::Buffer& buffer, vk::DeviceMemory memory, uint64_t offset)
{
    mDevice.bindBufferMemory(buffer, memory, offset);
}


void    RenderDevice::bindImageMemory(vk::Image& image, vk::DeviceMemory memory, const uint64_t offset)
{
    mDevice.bindImageMemory(image, memory, offset);
}

