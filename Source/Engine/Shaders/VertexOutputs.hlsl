
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
	float4 curPosition : POSITION2;
	float4 prevPosition : POSITION1;
	float4 positionWS : POSITION0;
	float2 uv : TEXCOORD0;
	float4 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float4 colour : COLOUR0;
	uint materialIndex : MATERIALID;
	uint materialFlags : MATERIAL_TYPES;
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
	uint materialIndex : MATERIALID;
	uint materialFlags : MATERIAL_TYPES;
};

struct ShadowMapVertOutput
{
	float4 position : SV_Position;
	float4 positionVS : POSITIONVS;
	float2 uv : TEXCOORD0;
	uint materialIndex : MATERIALID;
	uint materialFlags : MATERIAL_TYPES;
};

struct VoxalizeGeometryOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
	float3 positionWS : POSITIONWS;
	float4 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float4 colour : COLOR0;
	uint materialIndex : MATERIALID;
	uint materialFlags : MATERIAL_TYPES;
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
	float4 tangent : TANGENT0;
	float4 colour : COLOR0;
};

struct SkinnedVertex
{
	float4 position : POSITION;
	float2 uv : TEXCOORD0;
	float4 normal : NORMAL0;
	float4 tangent : TANGENT0;
	float4 colour : COLOR0;
	uint4  boneIndicies : BONE_INDEX;
	float4 boneWeights : BONE_WEIGHTS;
};


struct BasicVertex
{
	float4 position : POSITION;
	float4 normal : NORMAL;
};

struct OverlayVertex
{
	float2 position : POSITION0;
	float2 uv : TEXCOORDS0;
	float4 albedo : COLOR0;
};

struct TerrainVertexOutput
{
	float4 position : SV_Position;
	float4 clipPosition : POSITION0;
	float4 prevClipPosition : POSITION1;
	float4 worldPosition : POSITION2;
	float3 normal : NORMAL;
};

struct MeshInstanceInfo
{
	uint transformsIndex;
	uint materialIndex;
	uint materialFlags;
	uint boneBufferOffset;
};

struct InstanceIDOutput
{
	float4 position : SV_Position;
	uint id : ID;
};

uint packNormals(const float4 normal)
{
    int4 Packed = int4(round(clamp(normal, -1.0, 1.0) * 127.0)) & 0xff;
    return uint(Packed.x | (Packed.y << 8) | (Packed.z << 16) | (Packed.w << 24));
}

float4 unpackNormals(const uint packedNormal)
{
    int SignedValue = int(packedNormal);
    int4 Packed = int4(SignedValue << 24, SignedValue << 16, SignedValue << 8, SignedValue) >> 24;
    return clamp(float4(Packed) / 127.0, -1.0, 1.0);
}

uint packColour(const float4 colour)
{
    return uint(colour.x * 255.0f) | (uint(colour.y * 255.0f) << 8) | (uint(colour.z * 255.0f) << 16) | (uint(colour.w * 255.0f) << 24);
}

float4 unpackColour(const uint colour)
{
    return float4((colour & 0xFF) / 255.0f, ((colour & 0xFF00) >> 8) / 255.0f, ((colour & 0xFF0000) >> 16) / 255.0f, ((colour & 0xFF000000) >> 24) / 255.0f);
}

Vertex readVertexFromBuffer(ByteAddressBuffer vertexBuffer, uint offset)
{
	Vertex vert;

	vert.position = asfloat(vertexBuffer.Load4(offset));
	vert.uv = asfloat(vertexBuffer.Load2(offset + 16));
	uint packedNormal = vertexBuffer.Load(offset + 24);
	vert.normal = unpackNormals(packedNormal);
	uint packedTangent = vertexBuffer.Load(offset + 28);
	vert.tangent = unpackNormals(packedTangent);
	vert.colour = unpackColour(vertexBuffer.Load(offset + 32));

	return vert;
}

void writeVertexToBuffer(RWByteAddressBuffer vertexBuffer, uint offset, Vertex vert)
{
	vertexBuffer.Store4(offset, asuint(vert.position));
	vertexBuffer.Store2(offset + 16, asuint(vert.uv));
	vertexBuffer.Store(offset + 24, packNormals(vert.normal));
	vertexBuffer.Store(offset + 28, packNormals(vert.tangent));
	vertexBuffer.Store(offset + 32, packColour(vert.colour));
}

void writeBasicVertexToBuffer(RWByteAddressBuffer vertexBuffer, uint offset, BasicVertex vert)
{
	vertexBuffer.Store4(offset, asuint(vert.position));
	vertexBuffer.Store(offset + 16, packNormals(vert.normal));
}