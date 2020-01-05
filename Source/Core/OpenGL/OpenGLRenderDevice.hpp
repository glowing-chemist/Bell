#ifndef GL_RENDER_DEVICE_HPP
#define GL_RENDER_DEVICE_HPP

#include "Core/RenderDevice.hpp"


struct GLFWwindow;


class OpenGLRenderDevice : public RenderDevice
{
public:
	OpenGLRenderDevice(GLFWwindow*);
	~OpenGLRenderDevice();

	virtual void					   generateFrameResources(RenderGraph&) override;

	virtual void                       startPass(const RenderTask&) override;
	virtual Executor*				   getPassExecutor() override;
	virtual void					   freePassExecutor(Executor*) override;
	virtual void					   endPass() override;

	virtual void                       startFrame() override;
	virtual void                       endFrame() override;

	virtual void                       destroyImage(ImageBase&) override;
	virtual void                       destroyImageView(ImageViewBase&) override {}

	virtual void                       destroyBuffer(BufferBase&) override;

	virtual void					   destroyShaderResourceSet(const ShaderResourceSetBase&) override {}

	virtual void					   setDebugName(const std::string&, const uint64_t handle, const uint64_t objectType) override;

	virtual void                       flushWait() const override;

	virtual void					   execute(BarrierRecorder&) override {} // barriers aren't a thing in GL.

	virtual void					   submitFrame() override {} // Work is submitted to the driver immediatly.
	virtual void					   swap() override;

	virtual size_t					   getMinStorageBufferAlignment() const override;

private:

	GLFWwindow* mWindow;

};

#endif