
float lineariseDepth(const float depth, const float near, const float far)
{
	return  (2.0f * near) / (far + near - depth * (far - near));
}