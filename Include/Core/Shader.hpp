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


class ShaderBase : public DeviceChild
{
public:

    ShaderBase(RenderDevice*, const std::string&, const uint64_t prefixHash);
    virtual ~ShaderBase() = default;

	virtual bool compile(const std::vector<std::string>& prefix = {}) = 0;
    virtual bool reload() = 0;
	inline bool hasBeenCompiled() const
	{
		return mCompiled;
	}

	std::string getFilePath() const
        { return mFilePath.string(); }

    uint64_t getPrefixHash() const
    {
        return mPrefixHash;
    }

protected:

    std::vector<char> mSource;
    std::vector<unsigned int> mSPIRV;

	fs::path mFilePath;

    bool mCompiled;

	fs::file_time_type mLastFileAccessTime;

    uint64_t mPrefixHash; // The prefix hash when this shader was compiled.
};


class Shader
{
public:

    Shader(RenderDevice*, const std::string&, const uint64_t prefixHash);
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

#endif
