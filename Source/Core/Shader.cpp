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
    mShaderStage{getShaderStage(mFilePath.string())},
    mCompiled{false}
{
    // Load the glsl Source from disk in to mGLSLSource.
    std::ifstream sourceFile{mFilePath};
    std::string source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
    mGLSLSource = std::move(source);

	mLastFileAccessTime = fs::last_write_time(path);
}


EShLanguage ShaderBase::getShaderStage(const std::string& path) const
{
    if(path.find(".vert") != std::string::npos)
        return EShLanguage::EShLangVertex;
    else if(path.find(".frag") != std::string::npos)
        return EShLanguage::EShLangFragment;
    else if(path.find(".comp") != std::string::npos)
        return EShLanguage::EShLangCompute;
    else if(path.find(".geom") != std::string::npos)
        return EShLanguage::EShLangGeometry;
    else if(path.find(".tesc") != std::string::npos)
        return EShLanguage::EShLangTessControl;
    else if(path.find(".tese") != std::string::npos)
        return EShLanguage::EShLangTessEvaluation;
    else
        return EShLanguage::EShLangFragment;
}


Shader::Shader(RenderDevice* dev, const std::string& path)
{
#ifdef VULKAN
	mBase = std::make_shared<VulkanShader>(dev, path);
#endif
}