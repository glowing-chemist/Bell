#include "VulkanShader.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#include <windows.h>
#include <dxc/dxcapi.h>



VulkanShader::VulkanShader(RenderDevice* device, const std::string& path, const uint64_t prefixHash) :
    ShaderBase{device, path, prefixHash},
	mShaderProfile{getShaderStage(mFilePath.string())}
{
}


VulkanShader::~VulkanShader()
{
	static_cast<VulkanRenderDevice*>(getDevice())->destroyShaderModule(mShaderModule);
}


bool VulkanShader::compile(const std::vector<ShaderDefine> &prefix)
{
    ShaderCompiler* compiler = getDevice()->getShaderCompiler();
    const wchar_t* args[] = {L"-spirv", L"-fspv-target-env=vulkan1.2", L"-O3"};
    IDxcBlob* binaryBlob = compiler->compileShader(mFilePath, prefix, mShaderProfile, &args[0], 3);
    BELL_ASSERT(binaryBlob != nullptr, "Failed to compile shader")

    const unsigned char* spirvBuffer = static_cast<const unsigned char*>(binaryBlob->GetBufferPointer());
    std::vector<unsigned int> SPIRV;
    SPIRV.resize(binaryBlob->GetBufferSize() / 4);
    memcpy(SPIRV.data(), spirvBuffer, SPIRV.size() * 4);

    {
        VulkanRenderDevice *device = static_cast<VulkanRenderDevice *>(getDevice());

        vk::ShaderModuleCreateInfo shaderModuleInfo{};
        shaderModuleInfo.setPCode(SPIRV.data());
        shaderModuleInfo.setCodeSize(SPIRV.size() * sizeof(uint32_t));
        mShaderModule = device->createShaderModule(shaderModuleInfo);

        device->setDebugName(mFilePath.string(), *reinterpret_cast<uint64_t *>(&mShaderModule),
                             VK_OBJECT_TYPE_SHADER_MODULE);

        mCompiled = true;
    }

    binaryBlob->Release();

    return true;
}


bool VulkanShader::reload()
{
    if (std::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
    {
        static_cast<VulkanRenderDevice*>(getDevice())->destroyShaderModule(mShaderModule);

        compile();
        return true;
    }

    return false;
}


const wchar_t* VulkanShader::getShaderStage(const std::string& path) const
{
    if (path.find(".vert") != std::string::npos)
        return L"vs_6_0";
    else if (path.find(".frag") != std::string::npos)
        return L"ps_6_0";
    else if (path.find(".comp") != std::string::npos)
        return L"cs_6_5";
    else if (path.find(".geom") != std::string::npos)
        return L"gs_6_0";
    else if (path.find(".tesc") != std::string::npos)
        return L"hs_6_0";
    else if (path.find(".tese") != std::string::npos)
        return L"ds_6_0";
    else
        return L"ps_6_0";
}
