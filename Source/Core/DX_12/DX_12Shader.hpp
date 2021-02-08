#ifndef DX_12_SHADER_HPP
#define DX_12_SHADER_HPP

#include "Core/Shader.hpp"

#include <d3d12.h>
#include <dxcapi.h>


class DX_12Shader : public ShaderBase
{
public:

	DX_12Shader(RenderDevice*, const std::string& path, const uint64_t prefixHash);
	~DX_12Shader();

	virtual bool compile(const std::vector<ShaderDefine> & prefix = {}) override final;
	virtual bool reload() override final;

	IDxcBlob* getShaderBlob()
	{
		return mBinaryBlob;
	}

private:

	IDxcBlob* mBinaryBlob;
	const WCHAR* mShaderProfile;

};

#endif