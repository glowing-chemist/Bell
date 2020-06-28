
// Vertex outputs.
struct UVVertOutput
{
	float2 uv : TEXCOORD0;
};

struct PositionAndUVVertOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

struct GBufferVertOutput
{
	float4 position : SV_Position;
	float4 positionWS : POSITION0;
	float2 uv : TEXCOORD0;
	float4 normal : NORMAL0;
	float4 colour : COLOUR0;
	uint materialIndex : MATERIALID;
	uint materialFlags : MATERIAL_FLAGS;
	float2 velocity : VELOCITY;
};

struct OverlayVertOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float4 albedo : COLOR0;
};

struct DepthOnlyOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	uint materialIndex;
	uint materialFlags;
};

struct ShadowMapVertOutput
{
	float4 position : SV_Position;
	float4 positionVS : POSITIONVS;
	float2 uv;
	uint materialIndex;
	uint materialFlags;
};

struct VoxalizeGeometryOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float3 positionWS : POSITIONWS;
	float4 normal : NORMAL0;
	uint materialID : MATERIALID;
};

struct PositionOutput
{
	float4 position : SV_Position;
};

// Vertex inputs.
struct Vertex
{
	float4 position : POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : NORMAL0;
	float4 colour : COLOR0;
};

struct OverlayVertex
{
	float2 position : POSITION0;
	float2 uv : TEXCOORDS0;
	float4 albedo : COLOR0;
};

struct ObjectMatracies
{
	float4x3 meshMatrix;
	float4x3 prevMeshMatrix;
	uint materialIndex;
	uint materialFlags;
};


void recreateMeshMatracies(in float4x3 meshMat, in float4x3 prevMeshMat, out float4x4 mat, out float4x4 prevMat)
{
	mat = float4x4( float4(meshMat[0], 0.0f), 
					float4(meshMat[1], 0.0f), 
					float4(meshMat[2], 0.0f),
					float4(meshMat[3], 1.0f));

	prevMat = float4x4( float4(prevMeshMat[0], 0.0f), 
						float4(prevMeshMat[1], 0.0f), 
						float4(prevMeshMat[2], 0.0f),
						float4(prevMeshMat[3], 1.0f));
}


void recreateMeshMatracies(ObjectMatracies objectinfo, out float4x4 mat, out float4x4 prevMat)
{
	mat = float4x4( float4(objectinfo.meshMatrix[0], 0.0f), 
									float4(objectinfo.meshMatrix[1], 0.0f), 
									float4(objectinfo.meshMatrix[2], 0.0f),
									float4(objectinfo.meshMatrix[3], 1.0f));

	prevMat = float4x4( float4(objectinfo.prevMeshMatrix[0], 0.0f), 
						float4(objectinfo.prevMeshMatrix[1], 0.0f), 
						float4(objectinfo.prevMeshMatrix[2], 0.0f),
						float4(objectinfo.prevMeshMatrix[3], 1.0f));
}
