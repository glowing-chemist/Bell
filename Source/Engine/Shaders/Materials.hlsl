#define DIELECTRIC_SPECULAR 0.04


float3 fresnelSchlickRoughness(const float cosTheta, const float3 F0, const float roughness)
{
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}


float2 parallaxUV(const float2 startUV, const float3 view, Texture2D<float> heightMap)
{
	const uint numSamples = 10;
	float2 size;
	heightMap.GetDimensions(size.x, size.y);

	float3 position = float3(startUV, 0.5f);
	const float2 xySize = float2(1.0f / size.x, 1.0f / size.y);
	const float2 normXY = normalize(view.xy);
	const float2 xyStepSize = xySize * normXY;
	const float3 stepDir = float3(xyStepSize, view.z * (length(xyStepSize) / length(view.xy)));

	for(uint i = 0; i < numSamples; ++i)
	{
		const float height = heightMap.SampleLevel(linearSampler, position.xy, 0.0f);

		if(position.z <= height)
			return position.xy;

		const float heightDiff = max(1.0f, 10.0f * abs(position.z - height));
		position += stepDir * heightDiff;
	}

	return startUV;
}


MaterialInfo calculateMaterialInfo(	const float4 vertexNormal,
									const float4 vertexColour,
									const uint materialIndex, 
									const float4 tangent,
									const float3 view,
									float2 uv)
{
	MaterialInfo mat;
	mat.diffuse = vertexColour;
	mat.normal = vertexNormal;
	mat.specularRoughness = float4(0.0f, 0.0f, 0.0f, 1.0f);
	mat.emissiveOcclusion = float4(0.0f, 0.0f, 0.0f, 1.0);

	uint nextMaterialSlot = 0;

	// calcaulte the tbn matrix used for normalmapping and parralax occlusion mapping.
	float3x3 tbv = tangentSpaceMatrix(vertexNormal, tangent);

#if SHADE_FLAGS & kMaterial_HeightMap
	{
		const float3 tangentView = mul(-view, transpose(tbv));
		Texture2D<float> heightMap = materials[materialIndex];

		uv = parallaxUV(uv, tangentView, heightMap);

		++nextMaterialSlot;
	}
#endif

#if SHADE_FLAGS & kMaterial_Diffuse
	{
		mat.diffuse = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv);
		++nextMaterialSlot;
	}
#endif

# if SHADE_FLAGS & kMaterial_Albedo
		++nextMaterialSlot;
#endif

#if SHADE_FLAGS & kMaterial_Normals
	{
		float3 normal = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).xyz;
		++nextMaterialSlot;

	    // remap normal
	    normal = remapNormals(normal);
	    normal = normalize(normal);

		{
	    	normal = mul(normal, tbv);

	    	normal = normalize(normal);
	    }

	    mat.normal = float4(normal, 1.0f);
	}
#endif

	float metalness = 0.0f;
#if SHADE_FLAGS & kMaterial_CombinedMetalnessRoughness
	{
		const float2 metalnessRoughness = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).zy;
		metalness = metalnessRoughness.x;
		mat.specularRoughness.w = metalnessRoughness.y;
		++nextMaterialSlot;
	}
#else
	{
#if 	SHADE_FLAGS & kMaterial_Roughness
		{
			mat.specularRoughness.w = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).x;
			++nextMaterialSlot;
		}
#endif

#if SHADE_FLAGS & kMaterial_Gloss
		{
			mat.specularRoughness.w = 1.0f - materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).x;
			++nextMaterialSlot;
		}
#endif

#if SHADE_FLAGS & kMaterial_Specular
		{
			mat.specularRoughness.xyz= materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).xyz;
			++nextMaterialSlot;
		}
#endif
		
#if SHADE_FLAGS & kMaterial_CombinedSpecularGloss
		{
			mat.specularRoughness = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv);
			mat.specularRoughness.w = 1.0f - mat.specularRoughness.w;
			++nextMaterialSlot;
		}
#endif

#if SHADE_FLAGS & kMaterial_Metalness
		{
			metalness = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).x;
			++nextMaterialSlot;
		}
#endif
	}
#endif

#if SHADE_FLAGS & kMaterial_Albedo
	{
		const float4 albedo = materials[materialIndex].Sample(linearSampler, uv);
		mat.diffuse = albedo * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalness);
		mat.diffuse.w = albedo.w;// Preserve the alpha chanle.

		const float NoV = dot(mat.normal.xyz, view);
        const float3 F0 = lerp(float3(DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR), albedo.xyz, metalness);
        mat.specularRoughness.xyz = fresnelSchlickRoughness(max(NoV, 0.0), F0, mat.specularRoughness.w);
	}
#endif

#if SHADE_FLAGS & kMaterial_AmbientOcclusion
	{
		mat.emissiveOcclusion.w = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).x;
		++nextMaterialSlot;
	}
#endif

#if SHADE_FLAGS & kMaterial_Emissive
	{
		mat.emissiveOcclusion.xyz = materials[materialIndex + nextMaterialSlot].Sample(linearSampler, uv).xyz;
	}
#endif


	return mat;
}


MaterialInfo calculateMaterialInfoRayTracing(	const float4 vertexColour,
														const uint materialTypes, 
														const uint materialIndex, 
														const float3 view, 
														const float2 uv,
														const float LOD)
{
	MaterialInfo mat;
	mat.diffuse = vertexColour;
	mat.normal = float4(0.0f, 1.0f, 0.0f, 0.0f);
	mat.specularRoughness = float4(0.0f, 0.0f, 0.0f, 1.0f);
	mat.emissiveOcclusion = float4(0.0f, 0.0f, 0.0f, 1.0);

	uint nextMaterialSlot = 0;

	if(materialTypes & kMaterial_Diffuse)
	{
		mat.diffuse = materials[materialIndex].SampleLevel(linearSampler, uv, LOD);
		++nextMaterialSlot;
	}

	if(materialTypes & kMaterial_Albedo)
		++nextMaterialSlot;

	if(materialTypes & kMaterial_Normals)
	{
		++nextMaterialSlot;
	}

	float metalness = 0.0f;
	if(materialTypes & kMaterial_CombinedMetalnessRoughness)
	{
		const float2 metalnessRoughness = materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).zy;
		metalness = metalnessRoughness.x;
		mat.specularRoughness.w = metalnessRoughness.y;
		++nextMaterialSlot;
	}
	else
	{
		if(materialTypes & kMaterial_Roughness)
		{
			mat.specularRoughness.w = materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).x;
			++nextMaterialSlot;
		}

		if(materialTypes & kMaterial_Gloss)
		{
			mat.specularRoughness.w = 1.0f - materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).x;
			++nextMaterialSlot;
		}

		if(materialTypes & kMaterial_Specular)
		{
			mat.specularRoughness.xyz= materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).xyz;
			++nextMaterialSlot;
		}
		
		if(materialTypes & kMaterial_CombinedSpecularGloss)
		{
			mat.specularRoughness = materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD);
			mat.specularRoughness.w = 1.0f - mat.specularRoughness.w;
			++nextMaterialSlot;
		}

		if(materialTypes & kMaterial_Metalness)
		{
			metalness = materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).x;
			++nextMaterialSlot;
		}
	}

	if(materialTypes & kMaterial_Albedo)
	{
		const float4 albedo = materials[materialIndex].SampleLevel(linearSampler, uv, LOD);
		mat.diffuse = albedo * (1.0 - DIELECTRIC_SPECULAR) * (1.0 - metalness);
		mat.diffuse.w = albedo.w;// Preserve the alpha chanle.

		const float NoV = dot(mat.normal.xyz, view);
        const float3 F0 = lerp(float3(DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR, DIELECTRIC_SPECULAR), albedo.xyz, metalness);
        mat.specularRoughness.xyz = fresnelSchlickRoughness(max(NoV, 0.0), F0, mat.specularRoughness.w);
	}

	if(materialTypes & kMaterial_AmbientOcclusion)
	{
		mat.emissiveOcclusion.w = materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).x;
		++nextMaterialSlot;
	}

	if(materialTypes & kMaterial_Emissive)
	{
		mat.emissiveOcclusion.xyz = materials[materialIndex + nextMaterialSlot].SampleLevel(linearSampler, uv, LOD).xyz;
	}


	return mat;
}
