
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
	uint materialID;
	float2 velocity;
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

// Vertex inputs.
struct Vertex
{
	float4 position : POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : NORMAL0;
	uint materialID;
};

struct OverlayVertex
{
	float2 position : POSITION0;
	float2 uv : TEXCOORDS0;
	float4 albedo : COLOR0;
};

struct ObjectMatracies
{
	float4x4 meshMatrix;
	float4x4 prevMeshMatrix;
};