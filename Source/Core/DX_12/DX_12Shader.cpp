#include "Core/DX_12/DX_12Shader.hpp"
#include "Core/BellLogging.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>

namespace 
{
    class includHandler : public IDxcIncludeHandler
    {
    public:

        includHandler() = default;

        virtual HRESULT STDMETHODCALLTYPE LoadSource(
            _In_ LPCWSTR pFilename,                                   // Candidate filename.
            _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
        )
        {
            IDxcLibrary* library;
            HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
            BELL_ASSERT(SUCCEEDED(hr), "Failed to create library")

            uint32_t codePage = CP_UTF8;
            IDxcBlobEncoding* sourceBlob;
            hr = library->CreateBlobFromFile(pFilename, &codePage, &sourceBlob);

            BELL_ASSERT(ppIncludeSource, "Invalid ptr")
            *ppIncludeSource = sourceBlob;

            library->Release();

            return hr;
        }

        //Stub these.
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override final
        {
            BELL_TRAP;

            return S_OK;
        };

        virtual ULONG STDMETHODCALLTYPE AddRef(void) override final 
        {
            return 0ULL;
        }

        virtual ULONG STDMETHODCALLTYPE Release(void) override final
        {
            return 0ULL;
        }

    private:


    };
}

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
    IDxcLibrary* library;
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    BELL_ASSERT(SUCCEEDED(hr), "Failed to create library")

    IDxcCompiler* compiler;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    BELL_ASSERT(SUCCEEDED(hr), "Failed to create shader compiler insatnce")

    uint32_t codePage = CP_UTF8;
    IDxcBlobEncoding* sourceBlob;
    // TODO Find a way to load from mSource rather than load twice.
    hr = library->CreateBlobFromFile(mFilePath.wstring().data(), &codePage, &sourceBlob);

    includHandler includer;
    IDxcOperationResult* result;
    hr = compiler->Compile(
        sourceBlob, // pSource
        mFilePath.wstring().data(), // pSourceName
        L"main", // pEntryPoint
        mShaderProfile, // pTargetProfile
        NULL, 0, // pArguments, argCount
        NULL, 0, // pDefines, defineCount
        &includer, // pIncludeHandler
        &result); // ppResult
    if (SUCCEEDED(hr))
        result->GetStatus(&hr);
    if (FAILED(hr))
    {
        if (result)
        {
            IDxcBlobEncoding* errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if (SUCCEEDED(hr) && errorsBlob)
            {
                BELL_LOG_ARGS("Compilation failed with errors:\n%s\n",
                    (const char*)errorsBlob->GetBufferPointer())
            }
        }
        
        library->Release();
        compiler->Release();
        sourceBlob->Release();
        result->Release();

        return false;
    }

    hr = result->GetResult(&mBinaryBlob);

    library->Release();
    compiler->Release();
    sourceBlob->Release();
    result->Release();

    return SUCCEEDED(hr);
}


bool DX_12Shader::reload()
{
    if (std::filesystem::last_write_time(mFilePath) > mLastFileAccessTime)
    {
        mBinaryBlob->Release();

        // Reload the modified source
        std::ifstream sourceFile{ mFilePath };
        std::vector<char> source{ std::istreambuf_iterator<char>(sourceFile), std::istreambuf_iterator<char>() };
        mSource = std::move(source);

        compile();
        return true;
    }

    return false;
}