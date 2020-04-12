#ifndef DEFAULTRESOURCESLOTS_HPP
#define DEFAULTRESOURCESLOTS_HPP


constexpr const char* kGBufferDepth	    = "GbufferDepth";
constexpr const char* kGBufferNormals   = "GBufferNormals";
constexpr const char* kGBufferAlbedo	    = "GBufferAlbedo";
constexpr const char* kGBufferSpecular   = "GBufferSpecular";
constexpr const char* kGBufferMaterialID = "GBUfferMaterialID";
constexpr const char* kGBufferUV	    = "GBUfferUV";
constexpr const char* kGBufferMetalnessRoughness = "GBufferMetalnessRoughness";
constexpr const char* kGBufferVelocity = "GBufferVelocity";
constexpr const char* kSSAO		    = "SSAO";
constexpr const char* kSSAOBuffer		= "SSAOBuf";
constexpr const char* kDefaultSampler    = "defaultSampler";
constexpr const char* kBlinnPhongLighting= "BlinnPhongLight";
constexpr const char* kMaterials		 = "Materials";
constexpr const char* kSkyBox			 = "SkyBox";
constexpr const char* kConvolvedSkyBox   = "ConvolvedSB";
constexpr const char* kActiveFroxels	 = "ActiveFroxels";
constexpr const char* kActiveFroxelBuffer = "ActiveFroxelBuffer";
constexpr const char* kShadowMap          = "ShadowMap";
constexpr const char* kShadowingLights    = "ShadowingLights";
constexpr const char* kCompositeOutput = "CompositeOutput";
constexpr const char* kTAAHistory		= "TAAHistory";
constexpr const char* kNewTAAHistory		= "NewTAAHistory";
constexpr const char* kLinearDepth		= "LinearDepth";
constexpr const char* kReflectionMap = "ReflectionMap";

constexpr const char* kCameraBuffer	    = "CameraBuffer";
constexpr const char* kLightBuffer	    = "LightBuffer";
constexpr const char* kMaterialMappings = "MaterialMappings";
constexpr const char* kSparseFroxels    = "SparseFroxles";
constexpr const char* kLightIndicies    = "LightIndicies";
constexpr const char* kSceneVertexBuffer = "SceneVertexBuffer";
constexpr const char* kSceneIndexBuffer  = "SceneIndexBuffer";

constexpr const char* kFrameBufer        = "FrameBuffer";
constexpr const char* kGlobalLighting	 = "GlobalLighting";
constexpr const char* kAnalyticLighting  = "AnalyticLighting";
constexpr const char* kOverlay			 = "Overlay";

constexpr const char* kDFGLUT			= "DFG";
constexpr const char* kLTCMat           = "LTCMat";
constexpr const char* kLTCAmp           = "LTCAmp";

constexpr const char* kDefaultFontTexture = "Fonts";

#endif
