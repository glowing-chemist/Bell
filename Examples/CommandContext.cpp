#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include "Engine/StaticMesh.h"
#include "Engine/Camera.hpp"
#include "Engine/UniformBuffers.h"
#include "Engine/Engine.hpp"
#include "Engine/DefaultResourceSlots.hpp"

#include "../ThirdParty/stb_image/stb_image.h"

struct TextureInfo
{
	std::vector<unsigned char> mData;
	int width;
	int height;
};

TextureInfo loadTexture(const std::string& filePath)
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filePath.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    std::vector<unsigned char> imageData{};
    imageData.resize(texWidth * texHeight * 4);

    std::memcpy(imageData.data(), pixels, texWidth * texHeight * 4);

    stbi_image_free(pixels);

	return {imageData, texWidth, texHeight};
}


void loadSkyBox(Image& skyBox, const std::vector<const char*> fileNames)
{
	uint32_t i = 0;
	for(const char* file : fileNames)
	{
		TextureInfo info = loadTexture(file);
		skyBox->setContents(info.mData.data(), 512, 512, 1, i);
		++i;
	}
}


std::array<float4, 16> generateNormalsOffsets()
{
	std::array<float4, 16> offsets;

	for(uint32_t i = 0; i < 16; ++i)
	{
		const float r1 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;
		const float r2 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;
		const float r3 = (static_cast <float> (rand()) / static_cast <float> (RAND_MAX / 2)) - 1.0f;

		const float3 vec = glm::normalize(float3{r1, r2, r3});

		offsets[i] = float4{vec, 0.0f};
	}

	return offsets;
}


int main()
{
    const uint32_t windowWidth = 800;
    const uint32_t windowHeight = 600;

	bool firstFrame = true;

    // initialse glfw
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // only resize explicitly


    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Bell Tests", nullptr, nullptr);
    if(window == nullptr) {
        glfwTerminate();
        return 5;
    }

	Engine engine{window};

	StaticMesh mesh("./oil_barrels_pbr/barrels_obj.obj"/* "./cube.obj"*/, VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material, 0);

	auto vertexData = mesh.getVertexData();
	auto IndexData = mesh.getIndexData();

	engine.startFrame();

	// get the planet tetxure.

	TextureInfo albedoTextureInfo = loadTexture("./oil_barrels_pbr/textures/drum3_base_color.png");
	Image albedoTexture = engine.createImage(albedoTextureInfo.height, albedoTextureInfo.height, 1, 1, 1, 1, Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, "Drum albedo");
	ImageView albedoTextureView{albedoTexture, ImageViewType::Colour};
	albedoTexture->setContents(albedoTextureInfo.mData.data(), albedoTextureInfo.height, albedoTextureInfo.height, 1);

	TextureInfo normalTextureInfo = loadTexture("./oil_barrels_pbr/textures/drum3_normal.png" );
	Image normalMappingTexture = engine.createImage(normalTextureInfo.height, normalTextureInfo.height, 1, 1, 1, 1, Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, "Drum Normals");
	ImageView normalMappingTextureView{normalMappingTexture, ImageViewType::Colour};
	normalMappingTexture->setContents(normalTextureInfo.mData.data(), normalTextureInfo.height, normalTextureInfo.height, 1);

	TextureInfo roughnessTextureInfo = loadTexture("./oil_barrels_pbr/textures/drum3_roughness.png");
	Image roughnessTexture = engine.createImage(roughnessTextureInfo.height, roughnessTextureInfo.height, 1, 1, 1, 1, Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, "Drum roughness");
	ImageView roughnessTextureView{roughnessTexture, ImageViewType::Colour};
	roughnessTexture->setContents(roughnessTextureInfo.mData.data(), roughnessTextureInfo.height, roughnessTextureInfo.height, 1);

	TextureInfo metalicTextureInfo = loadTexture("./oil_barrels_pbr/textures/drum3_metallic.png");
	Image metalicTexture = engine.createImage(metalicTextureInfo.height, metalicTextureInfo.height, 1, 1, 1, 1, Format::RGBA8UNorm, ImageUsage::Sampled | ImageUsage::TransferDest, "Drum metalness");
	ImageView metalicTextureView{metalicTexture, ImageViewType::Colour};
	metalicTexture->setContents(metalicTextureInfo.mData.data(), metalicTextureInfo.height, metalicTextureInfo.height, 1);

	Image skyBox = engine.createImage(512, 512, 1, 1, 6, 1, Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::TransferDest, "Sky Box");
	ImageView skyBoxView{skyBox, ImageViewType::Colour, 0, 6};
	const std::vector<const char*> skyBoxTextures{"./skybox/peaks_lf.tga", "./skybox/peaks_rt.tga", "./skybox/peaks_up.tga", "./skybox/peaks_dn.tga", "./skybox/peaks_ft.tga", "./skybox/peaks_bk.tga"};

	loadSkyBox(skyBox, skyBoxTextures);

	Image convolvedSkyBox = engine.createImage(512, 512, 1, 10, 6, 1, Format::RGBA8UNorm, ImageUsage::CubeMap | ImageUsage::Sampled | ImageUsage::Storage, "Convolved Sky Box");
	ImageView convoledMipChain(convolvedSkyBox, ImageViewType::Colour, 0, 6, 0, 10);
	std::vector<ImageView> convolderSkyBoxMips{};
	for(uint32_t i = 0; i < 10; ++i)
	{
		convolderSkyBoxMips.push_back(ImageView{convolvedSkyBox, ImageViewType::Colour, 0, 6, i});
	}

	Image integratedDFG = engine.createImage(512, 512, 1, 1, 1, 1, Format::RG16UNorm, ImageUsage::Sampled | ImageUsage::Storage, "Intergrated DFG");
	ImageView integratedDFGView{integratedDFG, ImageViewType::Colour};

	Buffer cameraBuffer = engine.createBuffer(sizeof (CameraBuffer), sizeof (CameraBuffer), BufferUsage::Uniform, "CameraBuffer");
	BufferView cameraBufferView{cameraBuffer};

	Buffer ssaoBuffer = engine.createBuffer(sizeof (SSAOBuffer), sizeof (SSAOBuffer), BufferUsage::Uniform, "SSAOBuf");
	BufferView	ssaoBufferView{ssaoBuffer};

	Buffer lightBuffer = engine.createBuffer(sizeof(Light), sizeof(Light), BufferUsage::Uniform, "LightBuf");
	BufferView lightBufferView{lightBuffer};

	Sampler linearSampler{SamplerType::Linear};

	Shader deferredVertexShader = engine.getShader("./Shaders/GBufferPassThroughMaterial.vert");
	Shader deferredFragmentShader = engine.getShader("./Shaders/GBufferMaterial.frag");

	Shader fullScreentriangleVertexShader = engine.getShader("./Shaders/FullScreenTriangle.vert");
	Shader SSAOFragmentShader = engine.getShader("./Shaders/SSAO.frag");

	Shader integratedDFGShader = engine.getShader("./Shaders/DFGLutGenerate.comp");

#define PBRLUT

#ifdef PBR
	Shader lightingShader = engine.getShader("./Shaders/AnalyticalIBLDeferredTexture.frag");
#endif

#ifdef PBRLUT
	Shader lightingShader = engine.getShader("./Shaders/DFGLUTIBLDeferredTexture.frag");
#endif

	Shader SkyboxVert = engine.getShader("./Shaders/SkyBox.vert");
	Shader SkyBoxFRag = engine.getShader("./Shaders/SkyBox.frag");

	Shader skyBoxConvole = engine.getShader("./Shaders/SkyBoxConvolve.comp");

	GraphicsPipelineDescription deferredDesc(deferredVertexShader, deferredFragmentShader,
											Rect{windowWidth, windowHeight},
											Rect{windowWidth, windowHeight});
	deferredDesc.mUseBackFaceCulling = true;
	deferredDesc.mDepthWrite = true;
	deferredDesc.mDepthTest = DepthTest::LessEqual;

	GraphicsPipelineDescription SSAODesc(fullScreentriangleVertexShader, SSAOFragmentShader,
										Rect{windowWidth, windowHeight},
										Rect{windowWidth, windowHeight});

	GraphicsPipelineDescription lightingDesc{fullScreentriangleVertexShader, lightingShader,
											Rect{windowWidth, windowHeight},
											Rect{windowWidth, windowHeight}};

	GraphicsPipelineDescription skyBoxDesc{SkyboxVert, SkyBoxFRag,
											Rect{windowWidth, windowHeight},
											Rect{windowWidth, windowHeight}};
	skyBoxDesc.mDepthTest = DepthTest::Equal;
	skyBoxDesc.mDepthWrite = false;

	Camera camera{{1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, 0.1f, 200.0f};

	CameraBuffer buf{};

	const auto offsets = generateNormalsOffsets();
	SSAOBuffer ssaoBuf{};

	std::memcpy(&ssaoBuf.mOffsets[0], offsets.data(), sizeof(float4) * offsets.size());

	ssaoBuf.mScale = 0.001f;
	ssaoBuf.mOffsetsCount = 16;

	MapInfo ssaoMapInfo{};
	ssaoMapInfo.mSize = sizeof(SSAOBuffer);
	ssaoMapInfo.mOffset = 0;

	void* ssaoBufPtr = ssaoBuffer->map(ssaoMapInfo);

		std::memcpy(ssaoBufPtr, &ssaoBuf, sizeof(SSAOBuffer));

	ssaoBuffer->unmap();

	Light light{};
	light.albedo = glm::vec4(1.0f);
	light.position = glm::vec4(camera.getPosition(), 1.0f);

	MapInfo lightMapInfo{};
	lightMapInfo.mSize = sizeof(Light);
	lightMapInfo.mOffset = 0;

	void* lightBufferPtr = lightBuffer->map(lightMapInfo);

		std::memcpy(lightBufferPtr, &light, sizeof(Light));

	lightBuffer->unmap();

	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();


		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camera.moveForward(0.5f);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camera.moveBackward(0.5f);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camera.moveLeft(0.5f);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camera.moveRight(0.5f);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			camera.rotateYaw(1.0f);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			camera.rotateYaw(-1.0f);
		if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			camera.moveUp((0.5f));
		if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
			camera.moveDown(0.5f);

		buf.mViewMatrix = camera.getViewMatrix();
		buf.mPerspectiveMatrix = camera.getPerspectiveMatrix();
		buf.mInvertedViewProjMatrix = glm::inverse(buf.mPerspectiveMatrix * buf.mViewMatrix);
		buf.mInvertedPerspective = glm::inverse(buf.mPerspectiveMatrix);
		buf.mNeaPlane = camera.getNearPlane();
		buf.mFarPlane = camera.getFarPlane();
		buf.mPosition = camera.getPosition();

		MapInfo info{};
		info.mOffset = 0;
		info.mSize = sizeof (CameraBuffer);
		void* cameraBufferPtr = cameraBuffer->map(info);

			std::memcpy(cameraBufferPtr, &buf, sizeof(CameraBuffer));

		cameraBuffer->unmap();


		if(!firstFrame)
			engine.startFrame();

		const ImageView& backBuffer = engine.getSwapChainImageView();

		engine.addMeshToBuffer(&mesh);

		CommandContext ctx;

		if(firstFrame)
		{
			ComputePipelineDescription skyBoxConvolveDesc{skyBoxConvole};

			const char* slots[] = {"convolved0", "convolved1", "convolved2", "convolved3", "convolved4", "convolved5", "convolved6", "convolved7", "convolved8", "convolved9"};
			ctx.setActiveContextType(ContextType::Compute);
			ctx.setCompuetPipelineState(skyBoxConvolveDesc);
			ctx.bindImageViews(&skyBoxView, &kSkyBox, 0, 1)
				.bindSamplers(&linearSampler, &kDefaultSampler, 1, 1)
				.bindStorageTextureViews(convolderSkyBoxMips.data(), slots, 2, convolderSkyBoxMips.size());
			ctx.dispatch(64, 64, 1);

			ctx.resetBindings();

#ifdef PBRLUT
			ComputePipelineDescription dfgGeneration{integratedDFGShader};

			ctx.setCompuetPipelineState(dfgGeneration);
			ctx.bindStorageTextureViews(&integratedDFGView, &kDFGLUT, 0, 1);
			ctx.dispatch(32, 32, 1);

			ctx.resetBindings();
#endif
		}

		// We can only draw this after the sky box has been convolved.
		//if(!firstFrame)
		//{
			const OutputAttachmentDesc deferredattachments[] = {{kGBufferNormals , Format::RGBA8UNorm, SizeClass::Swapchain, LoadOp::Clear_Black},
																{kGBufferUV , Format::RGBA32SFloat, SizeClass::Swapchain, LoadOp::Clear_Black},
															   {kGBufferMaterialID , Format::R32Uint, SizeClass::Swapchain, LoadOp::Clear_Black}};
			const OutputAttachmentDesc depthAttachment{kGBufferDepth, Format::D32Float, SizeClass::Swapchain, LoadOp::Clear_White};
			ctx.setActiveContextType(ContextType::Graphics);
			ctx.setGraphicsPipelineState(deferredDesc);
			ctx.setVertexAttributes(VertexAttributes::Position4 | VertexAttributes::Normals | VertexAttributes::TextureCoordinates | VertexAttributes::Material);
			ctx.bindUniformBufferViews(&cameraBufferView, &kCameraBuffer, 0, 1)
					.setPushConstantsEnable(true)
					.bindRenderTargets(deferredattachments, 0, 3)
					.bindDepthStencilView(&depthAttachment, 3, 1);
			ctx.setPushConstants(glm::mat4(1.0f));
			ctx.drawIndexed(0, 0, static_cast<uint32_t>(IndexData.size()));

			ctx.resetBindings();

			LoadOp frameBufferClear = LoadOp::Clear_Black;

			const ImageView skyBoxTextures[] = {skyBoxView, convoledMipChain};
			const char* skyBoxSlots[] = {kSkyBox, kConvolvedSkyBox};
			const char* gbufferSlots[] = {kGBufferDepth, kGBufferNormals, kGBufferMaterialID, kGBufferUV};

			ImageViewArray materials = {albedoTextureView, normalMappingTextureView, roughnessTextureView, metalicTextureView};
			ctx.setGraphicsPipelineState(lightingDesc);
			ctx.setVertexAttributes(0)
#ifdef PBRLUT
					.bindUniformBufferViews(&cameraBufferView, &kCameraBuffer, 0, 1)
					.bindImageViews(&integratedDFGView, &kDFGLUT, 1, 1)
					.bindImageViews(gbufferSlots, 2, 4)
					.bindImageViews(skyBoxTextures, skyBoxSlots, 6, 2)
					.bindSamplers(&linearSampler, &kDefaultSampler, 8, 1)
					.bindImageViewArrays(&materials, &kMaterials, 9, 1)
#else
					.bindUniformBufferViews(&cameraBufferView, &kCameraBuffer, 0, 1)
					.bindImageViews(gbufferSlots, 1, 4)
					.bindImageViews(skyBoxTextures, skyBoxSlots, 5, 2)
					.bindSamplers(&linearSampler, &kDefaultSampler, 7, 1)
					.bindImageViewArrays(&materials, &kMaterials, 8, 1)
#endif
					.setPushConstantsEnable(true)
					.bindRenderTargets(&backBuffer, &kFrameBufer, &frameBufferClear, 0, 1);

			ctx.setPushConstants(glm::mat4(1.0f));
			ctx.draw(0, 3);

			ctx.resetBindings();
		//}

		LoadOp frameBufferLoadOp = LoadOp::Preserve;
		const OutputAttachmentDesc depthAttachmentPreserve{kGBufferDepth, Format::D32Float, SizeClass::Custom, LoadOp::Preserve};
		ctx.setActiveContextType(ContextType::Graphics);
		ctx.setGraphicsPipelineState(skyBoxDesc);
		ctx.setVertexAttributes(0);
		ctx.bindUniformBufferViews(&cameraBufferView, &kCameraBuffer, 0, 1)
				.bindImageViews(&skyBoxView, &kSkyBox, 1, 1)
				.bindSamplers(&linearSampler, &kDefaultSampler, 2, 1)
				.bindRenderTargets(&backBuffer, &kFrameBufer, &frameBufferLoadOp, 0, 1)
				.bindDepthStencilView(&depthAttachmentPreserve, 1, 1);
		ctx.draw(0, 3);

		engine.submitCommandRecorder(ctx);
		engine.swap();
		engine.endFrame();
		engine.flushWait();

		firstFrame = false;
	}
}

