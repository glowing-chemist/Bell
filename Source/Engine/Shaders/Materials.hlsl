#define DIELECTRIC_SPECULAR 0.04


float3 fresnelSchlickRoughness(const float cosTheta, const float3 F0, const float roughness)
{
    return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


MaterialInfo calculateMaterialInfo(	const float4 vertexNormal, 
									const uint materialTypes, 
									const uint materialIndex, 
									const float3 view, 
									const float2 uv)
{
	const uint materialCount = countbits(materialTypes);

	MaterialInfo mat;
	mat.diffuse = float4(0.5f, 0.5f, 0.5f, 1.0f);
	mat.normal = vertexNormal;
	mat.specularRoughness = float4(0.5f, 0.5f, 0.5f, 0.5f);

	uint nextMaterialSlot = 0;

	if(materialTypes & kMaterial_Diffuse)
	{
		mat.diffuse = materials[materialIndex * materialCount].Sample(linearSampler, uv);
		++nextMaterialSlot;
	}

	if(materialTypes & kMaterial_Albedo)
		++nextMaterialSlot;

	if(materialTypes & kMaterial_Normals)
	{
		float3 normal = materials[materialIndex * materialCount + nextMaterialSlot].Sample(linearSampler, uv).xyz;
		++nextMaterialSlot;

	    // remap normal
	    normal = remapNormals(normal);
	    normal = normalize(normal);

	    const float2 xDerivities = ddx_fine(uv);
	    const float2 yDerivities = ddy_fine(uv);

		{
	    	float3x3 tbv = tangentSpaceMatrix(vertexNormal, view, float4(xDerivities, yDerivities));

	    	normal = mul(normal, tbv);

	    	normal = normalize(normal);
	    }

	    mat.normal = float4(normal, 1.0f);
	}

	if(materialTypes & kMaterial_Roughness)
	{
		mat.specularRoughness.w = materials[materialIndex * materialCount + nextMaterialSlot].Sample(linearSampler, uv).x;
		++nextMaterialSlot;
	}

	if(materialTypes & kMaterial_Gloss)
	{
		mat.specularRoughness.w = 1.0f - materials[materialIndex * materialCount + nextMaterialSlot].Sample(linearSampler, uv).x;
		++nextMaterialSlot;
	}

	if(materialTypes & kMaterial_Specular)
	{
		mat.specularRoughness.xyz = materials[materialIndex * materialCount + nextMaterialSlot].Sample(linearSampler, uv).xyz;
	}

	float metalness = 0.0f;
	if(materialTypes & kMaterial_Metalness)
	{
		metalness = materials[materialIndex * materialCount + nextMaterialSlot].Sample(linearSampler, uv).x;
	}

	if(materialTypes & kMaterial_Albedo)
	{
		const float4 albedo = materials[materialIndex * materialCount].Sample(linearSampler, uv);
		mat.diffuse = albedo * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalness);
		mat.diffuse.w = albedo.w;// Preserve teh alpha chanle.

		const float NoV = dot(mat.normal.xyz, view);
        const float3 F0 = lerp(float3(DIELECTRIC_SPECULAR), albedo.xyz, metalness);
        mat.specularRoughness.xyz = fresnelSchlickRoughness(max(NoV, 0.0), F0, mat.specularRoughness.w);
	}


	return mat;
}