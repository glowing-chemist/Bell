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
        mRootDir{path}
    {}

    std::optional<fs::path> render();


private:

    std::vector<fs::path> getChildren(const fs::path);
    std::optional<fs::path> renderChildren(const fs::path);

    fs::path mRootDir;
};

#endif // IMGUIFILEBROWSER_H
