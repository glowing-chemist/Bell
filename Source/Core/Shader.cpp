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
    mSource{},
    mFilePath{path},
    mCompiled{false}
{
    // Load the glsl Source from disk in to mGLSLSource.
	const auto fileSize = std::filesystem::file_size(path);
	mSource.resize(fileSize);

	FILE* file = fopen(path.c_str(), "r");
	fread(mSource.data(), sizeof(char), mSource.size(), file);
	fclose(file);

	mLastFileAccessTime = fs::last_write_time(path);
}


Shader::Shader(RenderDevice* dev, const std::string& path)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanShader>(dev, path);
#endif
}