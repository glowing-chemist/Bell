#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#ifdef VULKAN
#include "Core/Vulkan/VulkanShader.hpp"
#endif


ShaderBase::ShaderBase(RenderDevice* device, const std::string& path) :
    DeviceChild{device},
    mFilePath{path},
    mCompiled{false}
{
    // Load the glsl Source from disk in to mGLSLSource.
    std::ifstream sourceFile{mFilePath};
    std::string source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
    mGLSLSource = std::move(source);

	mLastFileAccessTime = fs::last_write_time(path);
}


Shader::Shader(RenderDevice* dev, const std::string& path)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanShader>(dev, path);
#endif
}