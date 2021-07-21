#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"
#include "Core/HashUtils.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#ifdef VULKAN
#include "Core/Vulkan/VulkanShader.hpp"
#endif

#ifdef DX_12
#include "Core/DX_12/DX_12Shader.hpp"
#endif


ShaderBase::ShaderBase(RenderDevice* device, const std::string& path) :
    DeviceChild{device},
    mFilePath{path},
    mCompiled{false},
    mCompileDefinesHash(0)
{
	mLastFileAccessTime = fs::last_write_time(path);
}


void ShaderBase::updateCompiledDefineHash(const std::vector<ShaderDefine>& defines)
{
    mCompileDefinesHash = 0;
    hash_combine(mCompileDefinesHash, defines);
}


Shader::Shader(RenderDevice* dev, const std::string& path)
{
#ifdef VULKAN
    mBase = std::make_shared<VulkanShader>(dev, path);
#endif

#ifdef DX_12
    mBase = std::make_shared<DX_12Shader>(dev, path, prefixHash);
#endif
}

// std::hash<ShaderDefine> definition
namespace std
{
    size_t hash<ShaderDefine>::operator()(const ShaderDefine& define) const noexcept
    {
        size_t hash = 0;
        hash_combine(hash, define.getName(), define.getValue());

        return hash;
    }
}
