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

    EditorNode(unsigned int id, const char* name, const uint64_t type, ImColor color = ImColor(255, 255, 255)):
        mID(id), mName(name), mColor(color), mType(type), mSize(0, 0)
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


class ImGuiNodeEditor
{

public:

    using NodeFactoryFunc = std::function<std::shared_ptr<EditorNode> (const uint64_t)>;

    ImGuiNodeEditor(const std::string&, NodeFactoryFunc);
    ~ImGuiNodeEditor();

    void addNode(const uint64_t);

    void draw();

private:

	inline void beginColumn()
	{
		ImGui::BeginGroup();
	}

	inline void nextColumn()
	{
		ImGui::EndGroup();
		ImGui::SameLine();
		ImGui::BeginGroup();
	}

	inline void endColumn()
	{
		ImGui::EndGroup();
	}

    const Pin& findPin(const ax::NodeEditor::PinId);

    std::string mName;

    // used to generate unique IDs.
    uint64_t mCurrentID;

    NodeFactoryFunc mNodeCreationFunction;

    ax::NodeEditor::EditorContext* mContext;

    std::vector<std::shared_ptr<EditorNode>> mNodes;
    std::vector<Link> mLinks;

};

#endif // IMGUINODEEDITOR_H
