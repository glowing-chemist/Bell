#include "Core/Shader.hpp"
#include "Core/RenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#include "SPIRV/GlslangToSpv.h"
#include "StandAlone/DirStackFileIncluder.h"

namespace
{
	// Taken from the glslang standalone tool.
	const TBuiltInResource defaultResources = {
	/* .MaxLights = */ 32,
	/* .MaxClipPlanes = */ 6,
	/* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
		/* .MaxFragmentImageUniforms = */ 8,
		/* .MaxCombinedImageUniforms = */ 8,
		/* .MaxGeometryTextureImageUnits = */ 16,
		/* .MaxGeometryOutputVertices = */ 256,
		/* .MaxGeometryTotalOutputComponents = */ 1024,
		/* .MaxGeometryUniformComponents = */ 1024,
		/* .MaxGeometryVaryingComponents = */ 64,
		/* .MaxTessControlInputComponents = */ 128,
		/* .MaxTessControlOutputComponents = */ 128,
		/* .MaxTessControlTextureImageUnits = */ 16,
		/* .MaxTessControlUniformComponents = */ 1024,
		/* .MaxTessControlTotalOutputComponents = */ 4096,
		/* .MaxTessEvaluationInputComponents = */ 128,
		/* .MaxTessEvaluationOutputComponents = */ 128,
		/* .MaxTessEvaluationTextureImageUnits = */ 16,
		/* .MaxTessEvaluationUniformComponents = */ 1024,
		/* .MaxTessPatchComponents = */ 120,
		/* .MaxPatchVertices = */ 32,
		/* .MaxTessGenLevel = */ 64,
		/* .MaxViewports = */ 16,
		/* .MaxVertexAtomicCounters = */ 0,
		/* .MaxTessControlAtomicCounters = */ 0,
		/* .MaxTessEvaluationAtomicCounters = */ 0,
		/* .MaxGeometryAtomicCounters = */ 0,
		/* .MaxFragmentAtomicCounters = */ 8,
		/* .MaxCombinedAtomicCounters = */ 8,
		/* .MaxAtomicCounterBindings = */ 1,
		/* .MaxVertexAtomicCounterBuffers = */ 0,
		/* .MaxTessControlAtomicCounterBuffers = */ 0,
		/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
		/* .MaxGeometryAtomicCounterBuffers = */ 0,
		/* .MaxFragmentAtomicCounterBuffers = */ 1,
		/* .MaxCombinedAtomicCounterBuffers = */ 1,
		/* .MaxAtomicCounterBufferSize = */ 16384,
		/* .MaxTransformFeedbackBuffers = */ 4,
		/* .MaxTransformFeedbackInterleavedComponents = */ 64,
		/* .MaxCullDistances = */ 8,
		/* .MaxCombinedClipAndCullDistances = */ 8,
		/* .MaxSamples = */ 4,
		/* .maxMeshOutputVerticesNV = */ 256,
		/* .maxMeshOutputPrimitivesNV = */ 512,
		/* .maxMeshWorkGroupSizeX_NV = */ 32,
		/* .maxMeshWorkGroupSizeY_NV = */ 1,
		/* .maxMeshWorkGroupSizeZ_NV = */ 1,
		/* .maxTaskWorkGroupSizeX_NV = */ 32,
		/* .maxTaskWorkGroupSizeY_NV = */ 1,
		/* .maxTaskWorkGroupSizeZ_NV = */ 1,
		/* .maxMeshViewCountNV = */ 4,

		/* .limits = */ {
			/* .nonInductiveForLoops = */ 1,
			/* .whileLoops = */ 1,
			/* .doWhileLoops = */ 1,
			/* .generalUniformIndexing = */ 1,
			/* .generalAttributeMatrixVectorIndexing = */ 1,
			/* .generalVaryingIndexing = */ 1,
			/* .generalSamplerIndexing = */ 1,
			/* .generalVariableIndexing = */ 1,
			/* .generalConstantMatrixVectorIndexing = */ 1,
		}};
}


Shader::Shader(RenderDevice* device, const std::string& path) :
    DeviceChild{device},
    mFilePath{path},
    mShaderStage{getShaderStage(mFilePath.string())}
{
    // Load the glsl Source from disk in to mGLSLSource.
    std::ifstream sourceFile{mFilePath};
    std::string source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
    mGLSLSource = std::move(source);

	mLastFileAccessTime = fs::last_write_time(path);
}


Shader::~Shader()
{
    if(mCompiled && release())
        getDevice()->destroyShaderModule(mShaderModule);
}


bool Shader::compile()
{
    glslang::TShader shader{mShaderStage};
    const char* shaderSourceCString = mGLSLSource.c_str();
    shader.setStrings(&shaderSourceCString, 1);

	glslang::EShTargetClientVersion VulkanClientVersion = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_0;
    shader.setEnvClient(glslang::EShClientVulkan, VulkanClientVersion);
	shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);

    TBuiltInResource Resources;
	Resources = defaultResources;
    EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);

	std::string preProcessedShader;

	DirStackFileIncluder includer{};
	includer.pushExternalLocalDirectory(fs::absolute(mFilePath.parent_path()).string());

	if(!shader.preprocess(&Resources, 100, ENoProfile, false, false, messages, &preProcessedShader, includer))
	{
		BELL_LOG_ARGS("Shader failed to preprocess: %s", shader.getInfoLog())

		return false;
	}

	const char* preprocessCString = preProcessedShader.c_str();
	shader.setStrings(&preprocessCString, 1);

    if(!shader.parse(&Resources, 100, false, messages))
	{
		BELL_LOG_ARGS("Shader failed to parse: %s", shader.getInfoLog())

        return false;
	}

    glslang::TProgram program;
    program.addShader(&shader);

    if(!program.link(messages))
        return false;

    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    glslang::GlslangToSpv(*program.getIntermediate(mShaderStage), mSPIRV, &logger, &spvOptions);

    vk::ShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.setPCode(mSPIRV.data());
    shaderModuleInfo.setCodeSize(mSPIRV.size() * 4);
    mShaderModule = getDevice()->createShaderModule(shaderModuleInfo);

    mCompiled = true;

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


