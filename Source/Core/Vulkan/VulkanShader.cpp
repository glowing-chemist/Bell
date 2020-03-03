#include "VulkanShader.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#include "SPIRV/GlslangToSpv.h"
#include "StandAlone/DirStackFileIncluder.h"


VulkanShader::VulkanShader(RenderDevice* device, const std::string& path) :
    ShaderBase{device, path}
{
}


VulkanShader::~VulkanShader()
{
	static_cast<VulkanRenderDevice*>(getDevice())->destroyShaderModule(mShaderModule);
}


bool VulkanShader::compile(const std::string& prefix)
{
    const bool compiled = ShaderBase::compile(prefix);
    BELL_ASSERT(compiled, "Failed to compile shader to SPIRV");

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vk::ShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.setPCode(mSPIRV.data());
    shaderModuleInfo.setCodeSize(mSPIRV.size() * 4);
    mShaderModule = device->createShaderModule(shaderModuleInfo);

	device->setDebugName(mFilePath.string(), *reinterpret_cast<uint64_t*>(&mShaderModule), VK_OBJECT_TYPE_SHADER_MODULE);

	mSPIRV.clear();

    return compiled;
}


bool VulkanShader::reload()
{
    if(std::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
	{
		static_cast<VulkanRenderDevice*>(getDevice())->destroyShaderModule(mShaderModule);

        // Reload the modified source
        std::ifstream sourceFile{mFilePath};
        std::string source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
        mGLSLSource = std::move(source);

        compile();
        return true;
    }

    return false;
}

