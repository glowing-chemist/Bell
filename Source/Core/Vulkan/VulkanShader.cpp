#include "VulkanShader.hpp"
#include "VulkanRenderDevice.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace
{
	class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:

		virtual shaderc_include_result* GetInclude(	const char* requested_source,
													shaderc_include_type type,
													const char* requesting_source,
													size_t include_depth) override final
		{
			const std::string headerPath = "./Shaders/" + std::string(requested_source);
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

				IncludeInfo entry{ shaderc_include_result{ headerPath.c_str(), headerPath.size(), data.c_str(), data.size(), nullptr }, std::move(data) };
				auto newEntry = mHeaderMap.insert({ headerPath, std::move(entry) }).first;

				return &newEntry->second.result;
			}
		}

		virtual void ReleaseInclude(shaderc_include_result* data)
		{
			// Don't need to do anything here, just free all resources at the end.
		}

		struct IncludeInfo
		{
			shaderc_include_result result;
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


bool VulkanShader::compile(const std::vector<std::string>& prefix)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetSourceLanguage(shaderc_source_language::shaderc_source_language_hlsl);
	options.SetOptimizationLevel(shaderc_optimization_level::shaderc_optimization_level_performance);
	for (const auto define : prefix)
		options.AddMacroDefinition(define);
	std::unique_ptr<shaderc::CompileOptions::IncluderInterface> includer = std::make_unique<ShaderIncluder>();
	options.SetIncluder(std::move(includer));
	options.SetTargetSpirv(shaderc_spirv_version::shaderc_spirv_version_1_3);
	options.SetTargetEnvironment(shaderc_target_env::shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);

	shaderc::SpvCompilationResult result = compiler.CompileGlslToSpv(mSource.data(), mSource.size(), mShaderStage, mFilePath.string().c_str(), "main", options);

	if (result.GetCompilationStatus() != shaderc_compilation_status::shaderc_compilation_status_success)
	{
		BELL_LOG("Shader compilation failed %s", result.GetErrorMessage().c_str())

		return false;
	}

	
	mSPIRV.insert(mSPIRV.begin(), result.cbegin(), result.cend());
	mCompiled = true;

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


shaderc_shader_kind VulkanShader::getShaderStage(const std::string& path) const
{
	if (path.find(".vert") != std::string::npos)
		return shaderc_vertex_shader;
	else if (path.find(".frag") != std::string::npos)
		return shaderc_fragment_shader;
	else if (path.find(".comp") != std::string::npos)
		return shaderc_compute_shader;
	else if (path.find(".geom") != std::string::npos)
		return shaderc_compute_shader;
	else if (path.find(".tesc") != std::string::npos)
		return shaderc_tess_control_shader;
	else if (path.find(".tese") != std::string::npos)
		return shaderc_tess_evaluation_shader;
	else
		return shaderc_fragment_shader;
}
