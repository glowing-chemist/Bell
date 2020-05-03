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

    if(mAlbedoOrDiffusePath != "")
    {
        if(mAlbedoOrDiffduseIndex == 0)
            flags |= static_cast<uint32_t>(MaterialType::Albedo);
        else
            flags |= static_cast<uint32_t>(MaterialType::Diffuse);
    }

    if(mNormalsPath != "")
    {
        flags |= static_cast<uint32_t>(MaterialType::Normals);
    }

    if(mRoughnessOrGlossPath != "")
    {
        if(mRoughnessGlossIndex == 0)
            flags |= static_cast<uint32_t>(MaterialType::Roughness);
        else
            flags |= static_cast<uint32_t>(MaterialType::Gloss);
    }

    if(mMetalnessOrSPecularPath != "")
    {
        if(mMetalnessSpecularIndex == 0)
            flags |= static_cast<uint32_t>(MaterialType::Metalness);
        else
            flags |= static_cast<uint32_t>(MaterialType::Specular);
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
        dropDown("Diffuse/Albedo", albedoOrDiffuse, 2, mAlbedoOrDiffduseIndex, mShowFileBrowserDiffuse);

        const char* normals[] = {"Normals"};
        uint8_t selectedNormals = 0;
        dropDown("Normals", normals, 1, selectedNormals, mShowFileBrowserNormals);

        const char* roughnessOrGloss[] = {"Roughness", "Gloss"};
        dropDown("Roughness/Gloss", roughnessOrGloss, 2, mRoughnessGlossIndex, mShowFileBrowserRoughness);

        const char* metalnessOrSpecular[] = {"Metalness", "Specular"};
        dropDown("Metalness/Specular", metalnessOrSpecular, 2, mMetalnessSpecularIndex, mShowFileBrowserMetalness);

        ImGui::InputText("Material name", mMaterialName, 32);

        if(ImGui::Button("Create"))
        {
            ImGui::End();

            return true;
        }

    }
    ImGui::End();

    if(mShowFileBrowserDiffuse || mShowFileBrowserNormals || mShowFileBrowserRoughness || mShowFileBrowserMetalness)
    {
        auto optionalPath = mFileBrowser.render();
        if(optionalPath)
        {

            if(mShowFileBrowserDiffuse)
            {
                mAlbedoOrDiffusePath = optionalPath->string();
                mShowFileBrowserDiffuse = false;
            }

            if(mShowFileBrowserNormals)
            {
                mNormalsPath = optionalPath->string();
                mShowFileBrowserNormals = false;
            }

            if(mShowFileBrowserRoughness)
            {
                mRoughnessOrGlossPath = optionalPath->string();
                mShowFileBrowserRoughness = false;
            }

            if(mShowFileBrowserMetalness)
            {
                mMetalnessOrSPecularPath = optionalPath->string();
                mShowFileBrowserMetalness = false;
            }
        }
    }

    return false;
}
