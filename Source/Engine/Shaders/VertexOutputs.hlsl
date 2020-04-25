
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
	uint materialID : MATERIALID;
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
	uint materialID;
};

struct ShadowMapVertOutput
{
	float4 position : SV_Position;
	float4 positionVS : POSITIONVS;
	float2 uv;
	uint materialID;
};

struct VoxalizeGeometryOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float3 positionWS : POSITIONWS;
	float4 normal : NORMAL0;
	uint materialID : MATERIALID;
};

// Vertex inputs.
struct Vertex
{
	float4 position : POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : NORMAL0;
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
	uint materialID;
	uint attributes;
};