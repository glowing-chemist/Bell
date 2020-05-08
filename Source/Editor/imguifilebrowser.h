#ifndef IMGUIFILEBROWSER_H
#define IMGUIFILEBROWSER_H

#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;


class ImGuiFileBrowser
{
public:
    ImGuiFileBrowser(const std::string& id, const fs::path& path) :
        mRootDir{path},
        mID{id}
    {}

    std::optional<fs::path> render();

private:

    std::vector<fs::path> getChildren(const fs::path);
    std::optional<fs::path> renderChildren(const fs::path);

    fs::path mRootDir;
    std::string mID;
};


enum TextureFilePaths
{
    AlbedoDiffusePath = 0,
    NormalsPath,
    RoughnessGlossPath,
    MetalnessSpecularPath,
    EmissivePath,
    AOPath,

    TextureFilePathCount
};

class ImGuiMaterialDialog
{
public:
    ImGuiMaterialDialog() :
        mFileBrowser("material", "/")
    {
        reset();
    }

    bool render();
    void reset()
    {
        memset(mMaterialName, 0, 32);

        for (uint32_t i = 0; i < TextureFilePathCount; ++i)
            mShowFileBrowser[i] = false;

        for (uint32_t i = 0; i < TextureFilePathCount; ++i)
            mPaths[i] = "";
    }

    uint32_t getMaterialFlags() const;

    const std::string& getPath(const TextureFilePaths pathType)
    {
        return mPaths[pathType];
    }

    const char* getMaterialName() const
    {
        return mMaterialName;
    }

private:

    uint8_t mAlbedoOrDiffduseIndex = 0;
    uint8_t mRoughnessGlossIndex = 0;
    uint8_t mMetalnessSpecularIndex = 0;

    bool mShowFileBrowser[TextureFilePathCount];
    ImGuiFileBrowser mFileBrowser;

    std::string mPaths[TextureFilePathCount];
    char mMaterialName[32];
};


#endif // IMGUIFILEBROWSER_H
