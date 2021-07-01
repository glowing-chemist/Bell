#include "ShaderCompiler.hpp"
#include "Core/BellLogging.hpp"

#include <windows.h>
#include <dxc/dxcapi.h>


namespace
{
    class shaderIncludeHandler : public IDxcIncludeHandler
    {
    public:

        shaderIncludeHandler(IDxcLibrary* library)  :
                mLibrary(library) {}

        virtual HRESULT LoadSource(
                _In_ const wchar_t* pFilename,                             // Candidate filename.
                _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource  // Resultant source object for included file, nullptr if not found.
        ) override final
        {
            uint32_t codePage = CP_UTF8;
            IDxcBlobEncoding* sourceBlob;
            HRESULT hr = mLibrary->CreateBlobFromFile(pFilename, &codePage, &sourceBlob);

            BELL_ASSERT(ppIncludeSource, "Invalid ptr")
            *ppIncludeSource = sourceBlob;

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

        IDxcLibrary* mLibrary;
    };
}

ShaderCompiler::ShaderCompiler()
{
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&mLibrary));
    BELL_ASSERT(SUCCEEDED(hr), "Failed to create library")

    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&mCompiler));
    BELL_ASSERT(SUCCEEDED(hr), "Failed to create shader compiler instance")
}

ShaderCompiler::~ShaderCompiler()
{
    mLibrary->Release();
    mCompiler->Release();
}


IDxcBlob* ShaderCompiler::compileShader(const std::filesystem::path& path,
                                        const std::vector<ShaderDefine>& prefix,
                                        const wchar_t* profile,
                                        const wchar_t** args,
                                        const uint32_t argCount)
{
    uint32_t codePage = CP_UTF8;
    IDxcBlobEncoding* sourceBlob;
    std::wstring wfilePath = path.wstring();
    HRESULT hr = mLibrary->CreateBlobFromFile(wfilePath.data(), &codePage, &sourceBlob);

    std::vector<DxcDefine> defines{};
    defines.reserve(prefix.size());
    for(auto& define : prefix)
    {
        defines.push_back(DxcDefine{define.getName().data(), define.getValue().data()});
    }

    shaderIncludeHandler includer(mLibrary);
    IDxcOperationResult* result;
    hr = mCompiler->Compile(
            sourceBlob, // pSource
            wfilePath.data(), // pSourceName
            L"main", // pEntryPoint
            profile, // pTargetProfile
            &args[0], 2, // pArguments, argCount
            defines.data(), defines.size(), // pDefines, defineCount
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

                fflush(stdout);
                BELL_TRAP;
            }
        }

        sourceBlob->Release();
        result->Release();

        return nullptr;
    }

    IDxcBlob* binaryBlob;
    hr = result->GetResult(&binaryBlob);
    BELL_ASSERT(SUCCEEDED(hr), "Failed to get shader binary")

    sourceBlob->Release();
    result->Release();

    return binaryBlob;
}