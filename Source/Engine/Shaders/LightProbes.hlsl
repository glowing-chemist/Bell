

struct SphericalHarmonic
{
	float4 position;
	float coefs[28]; // one extra to round to multiple of float4;
};


struct BasisCoefs
{
	float Y00;
	float Y11;
	float Y10;
	float Y1_1;
	float Y21;
	float Y2_1;
	float Y2_2;
	float Y20;
	float Y22;
};


BasisCoefs calculateBasis(float3 normal)
{
	BasisCoefs basis;
	basis.Y00   = 0.282095f;
	basis.Y11   = 0.488603f * normal.x;
	basis.Y10   = 0.488603f * normal.y;
	basis.Y1_1  = 0.488603f * normal.z;
	basis.Y21   = 1.092548f * (normal.x * normal.z);
	basis.Y2_1  = 1.092548f * (normal.y * normal.z);
	basis.Y2_2  = 1.092548f * (normal.x * normal.y);
	basis.Y20   = 0.315392f * ((3.0f * normal.z * normal.z) - 1.0f);
	basis.Y22   = 0.546274f * (normal.x * normal.x - normal.y * normal.y);

	return basis;
}

float3 calculateProbeIrradiance(float3 positionWS, float3 normal, SphericalHarmonic probe1, SphericalHarmonic probe2, SphericalHarmonic probe3)
{
	const float A0 = 3.141593f;
	const float A1 = 2.094395f;
	const float A2 = 0.785398f;

	const BasisCoefs localBasis = calculateBasis(normal);

	const float probe1Dist = length(positionWS - probe1.position);
	const float probe2Dist = length(positionWS - probe2.position);
	const float probe3Dist = length(positionWS - probe3.position); 
	const float totalDist = probe1Dist + probe2Dist + probe3Dist;

	const float probe1Coef = 1.0f - (probe1Dist / totalDist);
	const float probe2Coef = 1.0f - (probe2Dist / totalDist);
	const float probe3Coef = 1.0f - (probe3Dist / totalDist);

	float3 L00 = float3(probe1.coefs[0] * probe1Coef + probe2.coefs[0] * probe2Coef + probe3.coefs[0] * probe3Coef, probe1.coefs[1] * probe1Coef + probe2.coefs[1] * probe2Coef + probe3.coefs[1] * probe3Coef, probe1.coefs[2] * probe1Coef + probe2.coefs[2] * probe2Coef + probe3.coefs[2] * probe3Coef);
	float3 L11 = float3(probe1.coefs[3] * probe1Coef + probe2.coefs[3] * probe2Coef + probe3.coefs[3] * probe3Coef, probe1.coefs[4] * probe1Coef + probe2.coefs[4] * probe2Coef + probe3.coefs[4] * probe3Coef, probe1.coefs[5] * probe1Coef + probe2.coefs[5] * probe2Coef + probe3.coefs[5] * probe3Coef);
	float3 L10 = float3(probe1.coefs[6] * probe1Coef + probe2.coefs[6] * probe2Coef + probe3.coefs[6] * probe3Coef, probe1.coefs[7] * probe1Coef + probe2.coefs[7] * probe2Coef + probe3.coefs[7] * probe3Coef, probe1.coefs[8] * probe1Coef + probe2.coefs[8] * probe2Coef + probe3.coefs[8] * probe3Coef);
	float3 L1_1 = float3(probe1.coefs[9] * probe1Coef + probe2.coefs[9] * probe2Coef + probe3.coefs[9] * probe3Coef, probe1.coefs[10] * probe1Coef + probe2.coefs[10] * probe2Coef + probe3.coefs[10] * probe3Coef, probe1.coefs[11] * probe1Coef + probe2.coefs[11] * probe2Coef + probe3.coefs[11] * probe3Coef);
	float3 L21 = float3(probe1.coefs[12] * probe1Coef + probe2.coefs[12] * probe2Coef + probe3.coefs[12] * probe3Coef, probe1.coefs[13] * probe1Coef + probe2.coefs[13] * probe2Coef + probe3.coefs[13] * probe3Coef, probe1.coefs[14] * probe1Coef + probe2.coefs[14] * probe2Coef + probe3.coefs[14] * probe3Coef);
	float3 L2_1 = float3(probe1.coefs[15] * probe1Coef + probe2.coefs[15] * probe2Coef + probe3.coefs[15] * probe3Coef, probe1.coefs[16] * probe1Coef + probe2.coefs[16] * probe2Coef + probe3.coefs[16] * probe3Coef, probe1.coefs[17] * probe1Coef + probe2.coefs[17] * probe2Coef + probe3.coefs[17] * probe3Coef);
	float3 L2_2 = float3(probe1.coefs[18] * probe1Coef + probe2.coefs[18] * probe2Coef + probe3.coefs[18] * probe3Coef, probe1.coefs[19] * probe1Coef + probe2.coefs[19] * probe2Coef + probe3.coefs[19] * probe3Coef, probe1.coefs[20] * probe1Coef + probe2.coefs[20] * probe2Coef + probe3.coefs[20] * probe3Coef);
	float3 L20 = float3(probe1.coefs[21] * probe1Coef + probe2.coefs[21] * probe2Coef + probe3.coefs[21] * probe3Coef, probe1.coefs[22] * probe1Coef + probe2.coefs[22] * probe2Coef + probe3.coefs[22] * probe3Coef, probe1.coefs[23] * probe1Coef + probe2.coefs[23] * probe2Coef + probe3.coefs[23] * probe3Coef);
	float3 L22 = float3(probe1.coefs[24] * probe1Coef + probe2.coefs[24] * probe2Coef + probe3.coefs[24] * probe3Coef, probe1.coefs[25] * probe1Coef + probe2.coefs[25] * probe2Coef + probe3.coefs[25] * probe3Coef, probe1.coefs[26] * probe1Coef + probe2.coefs[26] * probe2Coef + probe3.coefs[26] * probe3Coef);

	// Reconstruct irradiance at this point.
	return float3(A0*localBasis.Y00*L00  
             + A1*localBasis.Y1_1*L1_1 + A1*localBasis.Y10*L10 + A1*localBasis.Y11*L11 
             + A2*localBasis.Y2_2*L2_2 + A2*localBasis.Y2_1*L2_1 + A2*localBasis.Y20*L20 + A2*localBasis.Y21*L21 + A2*localBasis.Y22*L22);
}
