#ifndef PASSTYPES_HPP
#define PASSTYPES_HPP

#include <cstdint>


#define PASS_TYPES  DepthPre = 1ULL, \
                    GBuffer = 1ULL << 1, \
                    ComputeSkinning = 1ULL << 2, \
                    Shadow = 1ULL << 3, \
                    CascadingShadow = 1ULL << 4, \
                    SSAO = 1ULL << 5, \
                    EditorDefault = 1ULL << 6, \
                    GBufferPreDepth = 1ULL << 7, \
                    InplaceCombine = 1ULL << 8, \
                    InplaceCombineSRGB = 1ULL << 9, \
                    Overlay = 1ULL << 10, \
                    Skybox = 1ULL << 11, \
                    ConvolveSkybox = 1ULL << 12, \
                    DFGGeneration = 1ULL << 13, \
                    Composite = 1ULL << 14, \
                    ForwardIBL = 1ULL << 15, \
                    LightFroxelation = 1ULL << 16, \
                    DeferredPBRIBL = 1ULL << 17, \
                    DeferredAnalyticalLighting = 1ULL << 18, \
                    ForwardAnalyticalLighting = 1ULL << 19, \
                    DeferredCombinedLighting = 1ULL << 20, \
                    ForwardCombinedLighting = 1ULL << 21, \
                    TAA = 1ULL << 22, \
                    LineariseDepth = 1ULL << 23, \
                    SSR = 1ULL << 24, \
                    Voxelize = 1ULL << 25, \
                    DebugAABB = 1ULL << 26, \
                    DebugWireFrame = 1ULL << 27, \
                    Transparent = 1ULL << 28, \
                    LightProbeDeferredGI = 1ULL << 29, \
                    VisualizeLightProbes = 1ULL << 30, \
                    OcclusionCulling = 1ULL << 31, \
                    PathTracing = 1ULL << 32, \
                    RayTracedReflections = 1ULL << 33, \
                    RayTracedShadows = 1ULL << 34, \
                    DownSampleColour = 1ULL << 35, \
                    VoxelTerrain = 1ULL << 36,   \
                    InstanceID = 1ULL << 37

// An enum to keep track of which 
enum class PassType : uint64_t
{
    PASS_TYPES

	// Add more as and when implemented.
};

inline const char* passToString(const PassType passType)
{
    switch(passType)
    {
        case PassType::DepthPre:
            return "Depth_Pre_pass";

        case PassType::GBuffer:
            return "GBuffer_pass";

        case PassType::SSAO:
            return "SSAO";

        case PassType::GBufferPreDepth:
            return "GBuffer_Pre_Depth";

		case PassType::InplaceCombine:
			return "Inplace_combine";

		case PassType::InplaceCombineSRGB:
			return "Inplace_combineSRGB";

        case PassType::DeferredPBRIBL:
            return "Deferred_PBR_IBL";

		case PassType::ForwardIBL:
			return "ForwardIBL";

		case PassType::ForwardCombinedLighting:
			return "Forward_combined_lighting";

		case PassType::Skybox:
			return "Skybox";

		case PassType::ConvolveSkybox:
			return "Convolve_skybox";

		case PassType::DFGGeneration:
			return "DFGGenration";

		case PassType::LightFroxelation:
			return "Light_froxelation";

		case PassType::DeferredAnalyticalLighting:
			return "Deferred_analytical_lighting";

        case PassType::Shadow:
            return "Shadow_Map";

		case PassType::TAA:
			return "TAA";

		case PassType::LineariseDepth:
			return "Linearise_depth";

		case PassType::Overlay:
			return "Overlay";

        case PassType::SSR:
            return "Screen_Space_Reflection";

        case PassType::Composite:
            return "Composite";

        case PassType::DebugAABB:
            return "DebugAABB";

        case PassType::DebugWireFrame:
            return "WireFrame";

        case PassType::Transparent:
            return "Transparent";

        case PassType::CascadingShadow:
            return "Cascade_Shadow_Map";

        case PassType::ComputeSkinning:
            return "Skinning";

        case PassType::Voxelize:
            return "Voxalize";

        case PassType::LightProbeDeferredGI:
            return "DeferredProbeLighting";

        case PassType::VisualizeLightProbes:
            return "VisulizeLightProbes";

        case PassType::OcclusionCulling:
            return "OcclusionCulling";

        case PassType::PathTracing:
            return "PathTracing";

        case PassType::RayTracedReflections:
            return "RayTraced_Reflections";

        case PassType::RayTracedShadows:
            return "RayTraced_Shadows";

        case PassType::DownSampleColour:
            return "DownSampled_Colour";

        case PassType::VoxelTerrain:
            return "Voxel_Terrain";

        default:
            return "UNKNOWN_PASS_TYPE";
    }
}


enum class AttachmentType
{
	Texture1D = 0,
	Texture2D,
	Texture3D,
    CubeMap,
	TextureArray,
	Sampler,
	Image1D,
	Image2D,
	Image3D,
	RenderTarget1D,
	RenderTarget2D,
	Depth,
	UniformBuffer,
    DataBufferRO,
    DataBufferWO,
    DataBufferRW,
	IndirectBuffer,
    VertexBuffer,
    IndexBuffer,
    CommandPredicationBuffer,
	PushConstants,
	ShaderResourceSet,
    TransferSource,
    TransferDestination
};


enum class Format
{
	RGBA8SRGB,
    RGB8UNorm,
    RGB8SRGB,
	RGB32SFloat,
	RGBA32SFloat,
	RGBA8UNorm,
	BGRA8UNorm,
	D32Float,
	D24S8Float,
	R32Uint,
	R32Float,
	R8UNorm,
    R8Norm,
    RG8UNorm,
	RG16UNorm,
	RG16SNorm,
	RGB16UNorm,
	RGBA16UNorm,
	RGBA16SNorm,
	RGBA16Float,
	RG32Float,
    RG16Float,
	RGBA32Float,
	R16UInt,
    RGBA16UInt,
    RGBA16Int,
    RGBA8Uint
};


enum class ImageUsage : uint32_t
{
	Sampled = 1,
	ColourAttachment = 1 << 1,
	DepthStencil = 1 << 2,
	Storage = 1 << 3,
	TransferDest = 1 << 4,
	TransferSrc = 1 << 5,
	CubeMap = 1 << 6,
	Transient = 1 << 7
};

inline ImageUsage operator|(const ImageUsage& lhs, const ImageUsage& rhs)
{
	return static_cast<ImageUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline uint32_t operator&(const ImageUsage& lhs, const ImageUsage& rhs)
{
	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}


enum class BufferUsage : uint32_t
{
	Vertex = 1,
	Index = 1 << 1,
	Uniform = 1 << 2,
	DataBuffer = 1 << 3,
	IndirectArgs = 1 << 4,
	TransferDest = 1 << 5,
    TransferSrc = 1 << 6,
    CommandPredication = 1 << 7,
    AccelerationStructure = 1 << 8
};

inline BufferUsage operator|(const BufferUsage& lhs, const BufferUsage& rhs)
{
	return static_cast<BufferUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline uint32_t operator&(const BufferUsage& lhs, const BufferUsage& rhs)
{
	return static_cast<uint32_t>(lhs)& static_cast<uint32_t>(rhs);
}


#endif

