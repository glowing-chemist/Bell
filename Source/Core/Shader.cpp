#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"

#include <fstream>
#include <iterator>

#include <SPIRV/GlslangToSpv.h>

// For some reason the header declaring this is missing the declaration on arches glallang??
// the shared library does conatin the symbol though so just declare it ourselves.
namespace glslang
{
    extern const TBuiltInResource DefaultTBuiltInResource;
}

thread_local bool Shader::mGLSLangInitialised = false;


Shader::Shader(RenderDevice* device, const std::string& path) :
    DeviceChild{device},
    mFilePath{path},
    mShaderStage{getShaderStage(mFilePath)}
{
    // Load the glsl Source from disk in to mGLSLSource.
    std::ifstream sourceFile{mFilePath};
    std::string source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
    mGLSLSource = std::move(source);

#ifdef _MSC_VER // std::filesystem is still experimental in msvc...
	mLastFileAccessTime = std::experimental::filesystem::last_write_time(path);
#else
    mLastFileAccessTime = std::filesystem::last_write_time(path);
#endif
}


Shader::~Shader()
{
    if(mCompiled)
        getDevice()->destroyShaderModule(mShaderModule);
}


bool Shader::compile()
{
    // Make sure the compiler process has been initialised.
    if(!mGLSLangInitialised)
    {
        glslang::InitializeProcess();
        mGLSLangInitialised = true;
    }

    glslang::TShader shader{mShaderStage};
    const char* shaderSourceCString = mGLSLSource.c_str();
    shader.setStrings(&shaderSourceCString, 1);

    glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;
    shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
    shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    TBuiltInResource Resources;
    Resources = glslang::DefaultTBuiltInResource;
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

    if(!shader.parse(&Resources, 100, false, messages))
        return false;

    glslang::TProgram program;
    program.addShader(&shader);

    if(!program.link(messages))
        return false;

    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    glslang::GlslangToSpv(*program.getIntermediate(mShaderStage), mSPIRV, &logger, &spvOptions);

    return true;
}


bool Shader::reload()
{
#ifdef _MSC_VER // MSVC still doesn't support std::filesystem ...
	if (std::experimental::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
#else
    if(std::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
#endif
	{
        // Reload the modified source
        std::ifstream sourceFile{mFilePath};
        std::string source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
        mGLSLSource = std::move(source);

        compile();
        return true;
    }

    return false;
}


const vk::ShaderModule&           Shader::getShaderModule() const
{
    return mShaderModule;
}


EShLanguage Shader::getShaderStage(const std::string& path) const
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
