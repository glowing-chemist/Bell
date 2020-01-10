#ifndef IMGUINODEEDITOR_H
#define IMGUINODEEDITOR_H

#include "imgui.h"
#include "imgui_node_editor.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>


class Engine;


enum class NodeTypes : uint64_t
{
    PASS_TYPES,
    Texture,
    Buffer
};


enum class PinType
{
    Texture,
    Buffer
};

enum class PinKind
{
    Output,
    Input
};

struct EditorNode;


struct Pin
{
    ax::NodeEditor::PinId       mID;
    std::weak_ptr<EditorNode>   mNode;
    std::string                 mName;
    PinType                     mType;
    PinKind                     mKind;

    Pin(unsigned int id, const std::shared_ptr<EditorNode>& parent,  const char* name, const PinType type, const PinKind kind):
        mID(id), mNode(parent), mName(name), mType(type), mKind(kind)
    {
    }
};


struct EditorNode
{

    ax::NodeEditor::NodeId mID;
    std::string      mName;
    std::vector<Pin> mInputs;
    std::vector<Pin> mOutputs;
    ImColor          mColor;
    uint64_t         mType;
    ImVec2           mSize;

    std::string State;
    std::string SavedState;

    virtual ~EditorNode() = default;

    virtual void draw() = 0;

    inline void beginColumn() const
    {
        ImGui::BeginGroup();
    }

    inline void nextColumn() const
    {
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginGroup();
    }

    inline void endColumn() const
    {
        ImGui::EndGroup();
    }

    EditorNode(const char* name, const uint64_t type, ImColor color):
        mID(0), mName(name), mColor(color), mType(type), mSize(0, 0)
    {}
};


struct Link
{
    ax::NodeEditor::LinkId mID;

    ax::NodeEditor::PinId mStartPinID;
    ax::NodeEditor::PinId mEndPinID;

    ImColor mColor;

    Link(ax::NodeEditor::LinkId id, ax::NodeEditor::PinId startPinId, ax::NodeEditor::PinId endPinId):
        mID(id), mStartPinID(startPinId), mEndPinID(endPinId), mColor(255, 255, 255)
    {
    }
};


struct PassNode : EditorNode
{
    PassNode(const char* name, const uint64_t type, ImColor color = ImColor(255, 255, 255)) :
        EditorNode(name, type, color)
    {}

    virtual void draw() override final;
};


struct ResourceNode : EditorNode
{
    ResourceNode(const char* name, const uint64_t type, ImColor color = ImColor(255, 255, 255)) :
        EditorNode(name, type, color)
    {
        // reserve some space to use as a buffer.
        mName.reserve(16);
    }

    virtual void draw() override final;
};


class ImGuiNodeEditor
{

public:

    using NodeFactoryFunc = std::function<std::shared_ptr<EditorNode> (const uint64_t)>;

    ImGuiNodeEditor(const std::string&, NodeFactoryFunc);
    ~ImGuiNodeEditor();

    void addNode(const uint64_t);
    void addNode(std::shared_ptr<EditorNode>&);

    void draw();

	void addPasses(Engine&);

    std::vector<std::string> getAvailableDebugTextures() const;

private:

    const Pin& findPin(const ax::NodeEditor::PinId);
	void generateLinks(const std::shared_ptr<EditorNode>&);

    std::string mName;

    // used to generate unique IDs.
    uint64_t mCurrentID;

    NodeFactoryFunc mNodeCreationFunction;

    ax::NodeEditor::EditorContext* mContext;

    std::vector<std::shared_ptr<EditorNode>> mNodes;
    std::vector<Link> mLinks;

};

#endif // IMGUINODEEDITOR_H
