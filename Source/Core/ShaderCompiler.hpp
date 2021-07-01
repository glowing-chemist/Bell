#ifndef SHADER_COMPILER_HPP
#define SHADER_COMPILER_HPP

#include <string>
#include <filesystem>
#include <unordered_map>
#include "Core/Shader.hpp"

class IDxcLibrary;
class IDxcCompiler;
class IDxcBlob;

class ShaderCompiler
{
public:
    ShaderCompiler();
    ~ShaderCompiler();


    IDxcBlob* compileShader(const std::filesystem::path& path,
                            const std::vector<ShaderDefine>& defines,
                            const wchar_t* profile,
                            const wchar_t** args,
                            const uint32_t argCount);

private:

    IDxcLibrary* mLibrary;
    IDxcCompiler* mCompiler;
};

#endif