#ifndef IMGUINODEEDITOR_H
#define IMGUINODEEDITOR_H

#include "imgui.h"
#include "imgui_node_editor.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>


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

    Pin(unsigned int id, const std::shared_ptr<EditorNode>& parent,  const char* name, PinType type):
        mID(id), mNode(parent), mName(name), mType(type), mKind(PinKind::Input)
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

    EditorNode(unsigned int id, const char* name, const uint64_t type, ImColor color = ImColor(255, 255, 255)):
        mID(id), mName(name), mColor(color), mType(type), mSize(0, 0)
    {}
};


struct Link
{
    ax::NodeEditor::LinkId ID;

    ax::NodeEditor::PinId StartPinID;
    ax::NodeEditor::PinId EndPinID;

    ImColor Color;

    Link(ax::NodeEditor::LinkId id, ax::NodeEditor::PinId startPinId, ax::NodeEditor::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};


class ImGuiNodeEditor
{

public:

    using NodeFactoryFunc = std::function<std::shared_ptr<EditorNode> (const uint64_t)>;

    ImGuiNodeEditor(const std::string&, NodeFactoryFunc);
    ~ImGuiNodeEditor();

    void addNode(const uint64_t);

    void draw();

private:

    std::string mName;

    // used to generate unique IDs.
    uint64_t mCurrentID;

    NodeFactoryFunc mNodeCreationFunction;

    ax::NodeEditor::EditorContext* mContext;

    std::vector<std::shared_ptr<EditorNode>> mNodes;
    std::vector<Link> mLinks;

};

#endif // IMGUINODEEDITOR_H
