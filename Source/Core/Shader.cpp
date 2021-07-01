#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#ifdef VULKAN
#include "Core/Vulkan/VulkanShader.hpp"
#endif

#ifdef DX_12
#include "Core/DX_12/DX_12Shader.hpp"
#endif


ShaderBase::ShaderBase(RenderDevice* device, const std::string& path, const uint64_t prefixHash) :
    DeviceChild{device},
    mFilePath{path},
    mCompiled{false},
    mPrefixHash(prefixHash),
    mCompileDefinesHash(0)
{
	mLastFileAccessTime = fs::last_write_time(path);
}


Shader::Shader(RenderDevice* dev, const std::string& path, const uint64_t prefixHash)
{
#ifdef VULKAN
    mBase = std::make_shared<VulkanShader>(dev, path, prefixHash);
#endif

#ifdef DX_12
    mBase = std::make_shared<DX_12Shader>(dev, path, prefixHash);
#endif
}
