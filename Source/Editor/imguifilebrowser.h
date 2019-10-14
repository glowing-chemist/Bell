#ifndef IMGUIFILEBROWSER_H
#define IMGUIFILEBROWSER_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;


class ImGuiFileBrowser
{
public:
    ImGuiFileBrowser(const fs::path& path) :
    mCurrentDirectory{path}
    {
	populateChildren();
    }

    std::optional<fs::path> render();


private:

    void populateChildren();

    fs::path mCurrentDirectory;
    std::vector<fs::path> mChildren;
};

#endif // IMGUIFILEBROWSER_H
