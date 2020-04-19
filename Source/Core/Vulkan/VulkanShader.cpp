#include "VulkanShader.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

#include "SPIRV/GlslangToSpv.h"

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
	}
	};

	class ShaderIncluder : public glslang::TShader::Includer
	{
	public:

		virtual IncludeResult* includeSystem(const char*, const char*, size_t) override final
		{
			BELL_TRAP; // Shouldn't end up here.
			return nullptr;
		}

		virtual IncludeResult* includeLocal(const char* headerName, const char*, size_t) override final
		{
			const std::string headerPath = "./Shaders/" + std::string(headerName);
			auto it = mHeaderMap.find(headerPath);
			if (it != mHeaderMap.end())
			{
				return &(it->second.result);
			}
			else
			{
				std::ifstream fileStream(headerPath);
				std::string data{ (std::istreambuf_iterator<char>(fileStream)),
					std::istreambuf_iterator<char>() };

				BELL_ASSERT(data.c_str(), "Unable to read file");

				IncludeInfo entry{ IncludeResult{ headerPath, data.c_str(), data.size(), nullptr }, std::move(data) };
				auto newEntry = mHeaderMap.insert({ headerPath, std::move(entry) }).first;

				return &newEntry->second.result;
			}
		}

		virtual void releaseInclude(IncludeResult*)
		{
			// Don't need to do anything here, just free all resources at the end.
		}

		struct IncludeInfo
		{
			IncludeResult result;
			std::string data;
		};
		const std::map<std::string, IncludeInfo>& getIncludedFiles() const
		{
			return mHeaderMap;
		}

	private:

		std::map<std::string, IncludeInfo> mHeaderMap;

	};

}

VulkanShader::VulkanShader(RenderDevice* device, const std::string& path) :
    ShaderBase{device, path},
	mShaderStage{getShaderStage(mFilePath.string())}
{
}


VulkanShader::~VulkanShader()
{
	static_cast<VulkanRenderDevice*>(getDevice())->destroyShaderModule(mShaderModule);
}


bool VulkanShader::compile(const std::string& prefix)
{
	glslang::TShader shader{ mShaderStage };
	const char* shaderSources[] = { mSource.data() };
	shader.setPreamble(prefix.c_str());
	shader.setStrings(shaderSources, 1);
#ifdef VULKAN
	glslang::EShTargetClientVersion clientVersion = glslang::EShTargetVulkan_1_1;
#elif defined(OPENGL)
	glslang::EShTargetClientVersion clientVersion = glslang::EShTargetOpenGL_450;
#endif
	glslang::EShTargetLanguageVersion TargetVersion = glslang::EShTargetSpv_1_3;
	shader.setEnvInput(glslang::EShSource::EShSourceHlsl, mShaderStage, glslang::EShClientVulkan, clientVersion);
	shader.setEnvTarget(glslang::EShTargetSpv, TargetVersion);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");

	TBuiltInResource Resources;
	Resources = defaultResources;
	EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | EShMsgReadHlsl);

	std::string preProcessedShader;

	// Generate the spirv file path.
	std::hash<std::string> prefixHasher{};
	const uint64_t prefixHash = prefixHasher(prefix);
	std::filesystem::path spirvFile = mFilePath;
	spirvFile += '.';
	spirvFile += std::to_string(prefixHash);
	spirvFile += ".spv";

	ShaderIncluder includer{};
	if (!shader.preprocess(&Resources, 100, ENoProfile, false, false, messages, &preProcessedShader, includer))
	{
		BELL_LOG_ARGS("Shader failed to preprocess: %s", shader.getInfoLog())

			return false;
	}

	// Check if we already have a valid spirv module on disk, if we do just load it instead.
	if (std::filesystem::exists(spirvFile))
	{
		bool upToDate = true;
		const auto spirvWriteTime = std::filesystem::last_write_time(spirvFile);
		const auto sourceWriteTime = std::filesystem::last_write_time(mFilePath);
		upToDate = upToDate && (spirvWriteTime >= sourceWriteTime);
		for (const auto& [path, entry] : includer.getIncludedFiles())
		{
			upToDate = upToDate && (spirvWriteTime >= std::filesystem::last_write_time(path));
		}

		if (upToDate)
		{
			const auto fileSize = std::filesystem::file_size(spirvFile);
			BELL_ASSERT((fileSize % sizeof(unsigned int)) == 0, "Incorrect file size")
			mSPIRV.resize(fileSize / sizeof(unsigned int));

			FILE* spirvHandle = fopen(spirvFile.string().c_str(), "rb");
			fread(mSPIRV.data(), sizeof(unsigned int), mSPIRV.size(), spirvHandle);
			fclose(spirvHandle);

			mCompiled = true;
		}
	}
	if (!mCompiled)
	{
		const char* preprocessCString = preProcessedShader.c_str();
		shader.setStrings(&preprocessCString, 1);

		if (!shader.parse(&Resources, 100, false, messages))
		{
			BELL_LOG_ARGS("Shader failed to parse: %s", shader.getInfoLog())

			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(messages))
			return false;

		spv::SpvBuildLogger logger;
		glslang::SpvOptions spvOptions;
		glslang::GlslangToSpv(*program.getIntermediate(mShaderStage), mSPIRV, &logger, &spvOptions);

		// Write the spirv to disk so we don't have to compile it next time.
		FILE* spirvHandle = fopen(spirvFile.string().c_str(), "wb");
		fwrite(mSPIRV.data(), sizeof(unsigned int), mSPIRV.size(), spirvHandle);
		fclose(spirvHandle);

		mCompiled = true;
	}

#ifdef NDEBUG // get rid of the source when not in debug builds to save memory.
	mSource.clear();
#endif

    BELL_ASSERT(mCompiled, "Failed to compile shader to SPIRV");

	VulkanRenderDevice* device = static_cast<VulkanRenderDevice*>(getDevice());

    vk::ShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.setPCode(mSPIRV.data());
    shaderModuleInfo.setCodeSize(mSPIRV.size() * sizeof(unsigned int));
    mShaderModule = device->createShaderModule(shaderModuleInfo);

	device->setDebugName(mFilePath.string(), *reinterpret_cast<uint64_t*>(&mShaderModule), VK_OBJECT_TYPE_SHADER_MODULE);

	mSPIRV.clear();

	return mCompiled;
}


bool VulkanShader::reload()
{
    if(std::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
	{
		static_cast<VulkanRenderDevice*>(getDevice())->destroyShaderModule(mShaderModule);

        // Reload the modified source
        std::ifstream sourceFile{mFilePath};
        std::vector<char> source{std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>()};
        mSource = std::move(source);

        compile();
        return true;
    }

    return false;
}


EShLanguage VulkanShader::getShaderStage(const std::string& path) const
{
	if (path.find(".vert") != std::string::npos)
		return EShLanguage::EShLangVertex;
	else if (path.find(".frag") != std::string::npos)
		return EShLanguage::EShLangFragment;
	else if (path.find(".comp") != std::string::npos)
		return EShLanguage::EShLangCompute;
	else if (path.find(".geom") != std::string::npos)
		return EShLanguage::EShLangGeometry;
	else if (path.find(".tesc") != std::string::npos)
		return EShLanguage::EShLangTessControl;
	else if (path.find(".tese") != std::string::npos)
		return EShLanguage::EShLangTessEvaluation;
	else
		return EShLanguage::EShLangFragment;
}
