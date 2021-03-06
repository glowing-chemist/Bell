
float varianceOcclusionFactor(const float fragDepth, const float2 moments)
{
	float E_x2 = moments.y;                   
	float Ex_2 = moments.x * moments.x;                   
	float variance = E_x2 - Ex_2;                       
	float mD = fragDepth - moments.x;                   
	float mD_2 = mD * mD;                   
	float p = variance / (variance + mD_2);                   
	return max( p, float(fragDepth <= moments.x) );
}