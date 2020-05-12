#include "imguifilebrowser.h"
#include "Engine/Scene.h"

#include "imgui.h"


std::optional<fs::path> ImGuiFileBrowser::render()
{
    std::optional<fs::path> path{};

    if(ImGui::Begin("FileBrowser"))
    {
        if (ImGui::TreeNode(mRootDir.string().c_str()))
        {
            ImGui::Indent(10.0f);

            path = renderChildren(mRootDir);

            ImGui::TreePop();
        }
    }

    ImGui::End();

    return path;
}


std::vector<fs::path> ImGuiFileBrowser::getChildren(const fs::path path)
{
    std::vector<fs::path> children{};

    for(const auto& child : fs::directory_iterator(path))
    {
        children.emplace_back(child);
    }

    return children;
}


std::optional<fs::path> ImGuiFileBrowser::renderChildren(const fs::path path)
{
    const std::vector<fs::path> children = getChildren(path);
    std::optional<fs::path> foundPath{};

    ImGui::Indent(10.0f);

    for (const auto& child : children)
    {
        std::error_code ec;
        const bool is_file = fs::is_regular_file(child, ec);

        if (ImGui::TreeNodeEx(child.filename().string().c_str(), is_file ? ImGuiTreeNodeFlags_Bullet : 0))
        {
            if (is_file)
            {
                foundPath = child;
                ImGui::TreePop();
                break;
            }

            ImGui::TreePop();

            foundPath = renderChildren(child);
            if (foundPath)
            {
                ImGui::GetStateStorage()->SetInt(ImGui::GetID(foundPath->filename().string().c_str()), 0);
                break;
            }
        }
    }

    ImGui::Unindent(10.0f);

    return foundPath;
}


uint32_t ImGuiMaterialDialog::getMaterialFlags() const
{
    uint32_t flags = 0;

    if(mPaths[AlbedoDiffusePath] != "")
    {
        if(mAlbedoOrDiffduseIndex == 0)
            flags |= static_cast<uint32_t>(MaterialType::Albedo);
        else
            flags |= static_cast<uint32_t>(MaterialType::Diffuse);
    }

    if(mPaths[NormalsPath] != "")
    {
        flags |= static_cast<uint32_t>(MaterialType::Normals);
    }

    if(mPaths[RoughnessGlossPath] != "")
    {
        if (mRoughnessGlossIndex == 0)
            flags |= static_cast<uint32_t>(MaterialType::Roughness);
        else if (mRoughnessGlossIndex == 1)
            flags |= static_cast<uint32_t>(MaterialType::Gloss);
        else
            flags |= static_cast<uint32_t>(MaterialType::CombinedMetalnessRoughness);
    }

    if(mPaths[MetalnessSpecularPath] != "")
    {
        if(mMetalnessSpecularIndex == 0)
            flags |= static_cast<uint32_t>(MaterialType::Metalness);
        else if(mMetalnessSpecularIndex == 1)
            flags |= static_cast<uint32_t>(MaterialType::Specular);
        else
            flags |= static_cast<uint32_t>(MaterialType::CombinedSpecularGloss);
    }

    if (mPaths[EmissivePath] != "")
    {
        flags |= static_cast<uint32_t>(MaterialType::Emisive);
    }

    if (mPaths[AOPath] != "")
    {
        flags |= static_cast<uint32_t>(MaterialType::AmbientOcclusion);
    }


    return flags;
}


bool ImGuiMaterialDialog::render()
{
    if(ImGui::Begin("Material dialog"))
    {
        auto dropDown = [](const char* name, const char** items, const uint32_t itemCount, uint8_t& selected,  bool& enableBrowser)
        {
            if(ImGui::BeginCombo(name, items[selected]))
            {

                for(uint32_t item = 0; item < itemCount; ++item)
                {
                    if(ImGui::Selectable(items[item], selected == item))
                        selected = item;
                }

                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if(ImGui::Button("..."))
            {
                enableBrowser = true;
            }
        };

        const char* albedoOrDiffuse[] = {"Albedo", "Diffuse"};
        dropDown(mPaths[AlbedoDiffusePath].empty() ? "Diffuse/Albedo" : mPaths[AlbedoDiffusePath].c_str(), albedoOrDiffuse, 2, mAlbedoOrDiffduseIndex, mShowFileBrowser[AlbedoDiffusePath]);

        const char* normals[] = {"Normals"};
        uint8_t selectedNormals = 0;
        dropDown(mPaths[NormalsPath].empty() ? "Normals" : mPaths[NormalsPath].c_str(), normals, 1, selectedNormals, mShowFileBrowser[NormalsPath]);

        const char* roughnessOrGloss[] = {"Roughness", "Gloss", "RoughnessMetalnessCombined"};
        dropDown(mPaths[RoughnessGlossPath].empty() ? "Roughness/Gloss" : mPaths[RoughnessGlossPath].c_str(), roughnessOrGloss, 3, mRoughnessGlossIndex, mShowFileBrowser[RoughnessGlossPath]);

        const char* metalnessOrSpecular[] = {"Metalness", "Specular", "SpecularGlossCombined"};
        dropDown(mPaths[MetalnessSpecularPath].empty() ? "Metalness/Specular" : mPaths[MetalnessSpecularPath].c_str(), metalnessOrSpecular, 3, mMetalnessSpecularIndex, mShowFileBrowser[MetalnessSpecularPath]);

        const char* emissive[] = { "Emissive" };
        uint8_t selectedEmissive = 0;
        dropDown(mPaths[EmissivePath].empty() ? "Emissive" : mPaths[EmissivePath].c_str(), emissive, 1, selectedEmissive, mShowFileBrowser[EmissivePath]);

        const char* ao[] = { "AO" };
        uint8_t selectedAO = 0;
        dropDown(mPaths[AOPath].empty() ? "AO" : mPaths[AOPath].c_str(), ao, 1, selectedAO, mShowFileBrowser[AOPath]);

        ImGui::InputText("Material name", mMaterialName, 32);

        if(ImGui::Button("Create"))
        {
            ImGui::End();

            return true;
        }

    }
    ImGui::End();

    for (uint32_t i = 0; i < TextureFilePathCount; ++i)
    {
        if (mShowFileBrowser[i])
        {
            auto optionalPath = mFileBrowser.render();
            if (optionalPath)
            {
                mPaths[i] = optionalPath->string();
                mShowFileBrowser[i] = false;
            }
        }
    }

    return false;
}
