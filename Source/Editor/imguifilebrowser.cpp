#include "imguifilebrowser.h"

#include "imgui.h"


std::optional<fs::path> ImGuiFileBrowser::render()
{
    int selectedFile = -1;
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
        if (ImGui::TreeNode(child.filename().string().c_str()))
        {
            if (fs::is_regular_file(child))
            {
                foundPath = child;
                ImGui::TreePop();
                break;
            }

            ImGui::TreePop();

            foundPath = renderChildren(child);
            if (foundPath)
                break;
        }
    }

    ImGui::Unindent(10.0f);

    return foundPath;
}

