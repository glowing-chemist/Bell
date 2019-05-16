#include "imguifilebrowser.h"

#include "imgui.h"


namespace fs = std::filesystem;


std::optional<fs::path> ImGuiFileBrowser::render()
{
    ImGui::Begin("FileBrowser");

    int selectedFile = -1;

    ImGui::BeginGroup();
    for(uint i = 0; i < mChildren.size(); ++i)
    {
        const auto file = mChildren[i];

        ImGui::RadioButton(file.filename().c_str(), &selectedFile, i);
    }

    if(selectedFile != -1)
    {
        const auto& file = mChildren[selectedFile];

        if(fs::is_directory(file))
        {
            mCurrentDirectory = file;
            populateChildren();
            // Don't return a path if a directory was selected.
            selectedFile = -1;
        }
    }

    ImGui::EndGroup();

    ImGui::End();

    if(selectedFile != -1)
        return mChildren[selectedFile];

    return {};
}


void ImGuiFileBrowser::populateChildren()
{
    mChildren.clear();

    for(const auto& child : fs::directory_iterator(mCurrentDirectory))
    {
        mChildren.emplace_back(child);
    }
}
