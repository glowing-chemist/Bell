#ifndef IMGUIFILEBROWSER_H
#define IMGUIFILEBROWSER_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>


class ImGuiFileBrowser
{
public:
    ImGuiFileBrowser(const std::filesystem::path& path) :
    mCurrentDirectory{path}
    {
	populateChildren();
    }

    std::optional<std::filesystem::path> render();


private:

    void populateChildren();

    std::filesystem::path mCurrentDirectory;
    std::vector<std::filesystem::path> mChildren;
};

#endif // IMGUIFILEBROWSER_H
