#ifndef SHADER_HPP
#define SHADER_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"

#include <string>
#include <vector>
#include <filesystem>

#include <vulkan/vulkan.hpp>

#include "glslang/Public/ShaderLang.h"

namespace fs = std::filesystem;


class RenderDevice;


class Shader : DeviceChild, RefCount
{
public:

    Shader(RenderDevice*, const std::string&);
    ~Shader();

    bool compile();
    bool reload();
	inline bool hasBeenCompiled() const
	{
		return mCompiled;
	}

    const vk::ShaderModule&           getShaderModule() const;

	std::string getFilePath() const
        { return mFilePath.string(); }

private:

    EShLanguage getShaderStage(const std::string&) const;

    std::string mGLSLSource;
    std::vector<unsigned int> mSPIRV;
    vk::ShaderModule mShaderModule;

	fs::path mFilePath;
    EShLanguage mShaderStage;

    bool mCompiled;

	fs::file_time_type mLastFileAccessTime;
};


#endif
