#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>


struct Material
{
	uint32_t index;
	std::string Albedo;
	std::string Normal;
	std::string Roughness;
	std::string Metalness;
};


int main(int argc, char** argv)
{
	if(argc < 2)
	{
		std::cout << "missing args\n";
		return -1;
	}
	const std::string wavefrontFile = argv[1];
	const std::string materialFile = argv[2];

	// gather material info.
	// Map from material name to texture paths
	std::map<std::string, Material> materialMap;
	
	{

	std::ifstream materialStream{};
	materialStream.open(materialFile);

	std::string id;
	std::string name;
	std::string materialType;
	uint32_t index = 0;
	while(materialStream >> id)
	{
		if(id == "newmtl")
		{
			Material mat{};
			mat.index = index++;
			materialStream >> name;
			std::cout << "found material " << name << '\n';
			uint8_t textureFound = 0;
			while(materialStream >> materialType)
			{
				if(materialType == "map_Ka")
				{
					++textureFound;
					std::string metal;
					materialStream >> metal;
					mat.Metalness = metal;
					std::cout << "Found Metalness texture\n";
				}
				else if(materialType == "map_Kd")
				{
					++textureFound;
					std::string diffuse;
					materialStream >> diffuse;
					mat.Albedo = diffuse;
					std::cout << "Found albedo texture\n";
				}
				else if(materialType == "map_bump")
				{
					++textureFound;
					std::string normal;
					materialStream >> normal;
					mat.Normal = normal;
					std::cout << "Found normals texture\n";
				}
				else if(materialType == "map_Ns" || materialType == "map_NS")
				{
					++textureFound;
					std::string roughness;
					materialStream >> roughness;
					mat.Roughness = roughness;
					std::cout << "adding material " << name << '\n';
				}

				if(textureFound == 4)
					break;
			}

			materialMap[name] = mat;
		}
	}

	}

	// now parse obj file for materials per mesh.
	std::map<std::string, uint32_t> meshMaterialIndex;
	{
		std::string id;
		std::string name;

		std::ifstream meshStream{};
		meshStream.open(wavefrontFile);

		while(meshStream >> id)
		{
			if(id == "g")
			{
				meshStream >> name;
				while(meshStream >> id)
				{
					if(id == "usemtl")
					{
						std::string material;
						meshStream >> material;
						
						std::cout << "mesh " << name << " uses material " << material << '\n';
						
						if(materialMap.find(material) == materialMap.end())
							std::cout << "\n\n\n############## Error: mesh " << name <<" has no material ################# \n\n\n";

						Material mat = materialMap[material];
						meshMaterialIndex[name] = mat.index;
						break;
					}
				}
			}
		}
	}

	// now we've gathered all the info, write the mat file (for the Bell renderer).
	{
		std::ofstream matStream{};
		matStream.open(wavefrontFile + ".mat");

		matStream << meshMaterialIndex.size() << '\n';

		for(const auto&[meshName, materialIndex] : meshMaterialIndex)
		{
			matStream << meshName << ' ' << materialIndex << '\n';
		}

		std::vector<Material> materials{};
		for(const auto&[name, mat] : materialMap)
		{
			materials.push_back(mat);
		}
		std::sort(materials.begin(), materials.end(), [](const Material& lhs, const Material& rhs)
				{
					return lhs.index < rhs.index;
				});

		for(const auto& mat : materials)
		{
			matStream << "Material " << mat.index << '\n';
			matStream << "Albedo " << mat.Albedo << '\n';
			matStream << "Normal " << mat.Normal << '\n';
			matStream << "Roughness " << mat.Roughness << '\n';
			matStream << "Metalness " << mat.Metalness << '\n'; 
		}
	}

}
