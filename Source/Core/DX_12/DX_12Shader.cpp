#include "Core/DX_12/DX_12Shader.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

DX_12Shader::DX_12Shader(RenderDevice* dev, const std::string& path, const uint64_t prefixHash) :
	ShaderBase(dev, path, prefixHash)
{
    if (path.find(".vert") != std::string::npos)
        mShaderProfile = L"vs_6_0";
    else if (path.find(".frag") != std::string::npos)
        mShaderProfile = L"ps_6_0";
    else if (path.find(".comp") != std::string::npos)
        mShaderProfile = L"cs_6_0";
    else if (path.find(".geom") != std::string::npos)
        mShaderProfile = L"gs_6_0";
    else if (path.find(".tesc") != std::string::npos)
        mShaderProfile = L"hs_6_0";
    else if (path.find(".tese") != std::string::npos)
        mShaderProfile = L"ds_6_0";
    else
        mShaderProfile = L"ps_6_0";
}


DX_12Shader::~DX_12Shader()
{
    mBinaryBlob->Release();
}


bool DX_12Shader::compile(const std::vector<ShaderDefine>& prefix)
{
    ShaderCompiler* compiler = getDevice()->getShaderCompiler();
    const wchar_t* args[] = {L"-O3"};
    mBinaryBlob = compiler->compileShader(mFilePath, prefix, mShaderProfile, &args[0], 1);
    BELL_ASSERT(mBinaryBlob != nullptr, "Failed to compile shader")

    sourceBlob->Release();
    result->Release();

    return SUCCEEDED(hr);
}


bool DX_12Shader::reload()
{
    if (std::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
    {
        mBinaryBlob->Release();

        compile();
        return true;
    }

    return false;
}