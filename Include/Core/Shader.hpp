#ifndef SHADER_HPP
#define SHADER_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"

#include <string>
#include <vector>
#include <filesystem>

#include <vulkan/vulkan.hpp>

#include <glslang/Public/ShaderLang.h>


class RenderDevice;


class Shader : DeviceChild, RefCount
{
public:

    Shader(RenderDevice*, const std::string&);
    ~Shader();

    bool compile();
    bool reload();
    bool hasBeenCompiled() const;

    const vk::ShaderModule&           getShaderModule() const;

    const std::string& getFilePath() const
        { return mFilePath; }

private:

    EShLanguage getShaderStage(const std::string&) const;

    std::string mGLSLSource;
    std::vector<unsigned int> mSPIRV;
    vk::ShaderModule mShaderModule;

    std::string mFilePath;
    EShLanguage mShaderStage;

    bool mCompiled;
#ifdef _MSC_VER  // MSVC still doesn't support std::filesystem ...
	std::experimental::filesystem::file_time_type mLastFileAccessTime;
#else
    std::filesystem::file_time_type mLastFileAccessTime;
#endif
};


#endif
