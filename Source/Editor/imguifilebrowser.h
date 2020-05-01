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


class ImGuiMaterialDialog
{
public:
    ImGuiMaterialDialog() :
        mShowFileBrowserDiffuse(false),
        mShowFileBrowserNormals(false),
        mShowFileBrowserRoughness(false),
        mShowFileBrowserMetalness(false),
        mFileBrowser("material", "/")
    {
        memset(mMaterialName, 0, 32);
    }

    bool render();
    void reset()
    {
        mShowFileBrowserDiffuse = false;
        mShowFileBrowserNormals = false;
        mShowFileBrowserRoughness = false;
        mShowFileBrowserMetalness= false;

        mAlbedoOrDiffusePath = "";
        mNormalsPath = "";
        mRoughnessOrGlossPath = "";
        mMetalnessOrSPecularPath = "";
    }

    uint32_t getMaterialFlags() const;

    const std::string& getAlbedoOrDiffusePath() const
    {
        return mAlbedoOrDiffusePath;
    }

    const std::string& getNormalsPath() const
    {
        return mNormalsPath;
    }

    const std::string& getRoughnessOrGlossPath() const
    {
        return mRoughnessOrGlossPath;
    }

    const std::string& getMetalnessOrSPecularPath() const
    {
        return mMetalnessOrSPecularPath;
    }

    const char* getMaterialName() const
    {
        return mMaterialName;
    }

private:

    uint8_t mAlbedoOrDiffduseIndex = 0;
    uint8_t mRoughnessGlossIndex = 0;
    uint8_t mMetalnessSpecularIndex = 0;

    bool mShowFileBrowserDiffuse = false;
    bool mShowFileBrowserNormals = false;
    bool mShowFileBrowserRoughness = false;
    bool mShowFileBrowserMetalness = false;
    ImGuiFileBrowser mFileBrowser;

    std::string mAlbedoOrDiffusePath;
    std::string mNormalsPath;
    std::string mRoughnessOrGlossPath;
    std::string mMetalnessOrSPecularPath;
    char mMaterialName[32];
};


#endif // IMGUIFILEBROWSER_H
