#ifndef SHADER_HPP
#define SHADER_HPP

#include "Core/DeviceChild.hpp"
#include "Core/GPUResource.hpp"

#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;


class RenderDevice;

class ShaderDefine
{
public:
    ShaderDefine(const std::wstring& name, const uint64_t value) :
        mName(name),
        mValue(std::to_wstring(value)) {}

    const std::wstring& getName() const
    {
        return mName;
    }

    const std::wstring& getValue() const
    {
        return mValue;
    }

    private:

    std::wstring mName;
    std::wstring mValue;
};

class ShaderBase : public DeviceChild
{
public:

    ShaderBase(RenderDevice*, const std::string&);
    virtual ~ShaderBase() = default;

    virtual bool compile(const std::vector<ShaderDefine>& prefix = {}) = 0;
    virtual bool reload() = 0;
	inline bool hasBeenCompiled() const
	{
		return mCompiled;
	}

	std::string getFilePath() const
        { return mFilePath.string(); }

    uint64_t getCompiledDefinesHash() const
    {
        return mCompileDefinesHash;
    }

protected:

    void updateCompiledDefineHash(const std::vector<ShaderDefine>&);

	fs::path mFilePath;

    bool mCompiled;

	fs::file_time_type mLastFileAccessTime;

    uint64_t mCompileDefinesHash;
};


class Shader
{
public:

    Shader(RenderDevice*, const std::string&);
	~Shader() = default;

	ShaderBase* getBase()
	{
		return mBase.get();
	}

	const ShaderBase* getBase() const
	{
		return mBase.get();
	}

	ShaderBase* operator->()
	{
		return getBase();
	}

	const ShaderBase* operator->() const
	{
		return getBase();
	}

private:

	std::shared_ptr<ShaderBase> mBase;

};


// std::hash for shader define.
namespace std
{
    template<>
    struct hash<ShaderDefine>
    {
        size_t operator()(const ShaderDefine&) const noexcept;
    };
}

#endif
