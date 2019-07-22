#include "ImGuiNodeEditor.h"

ImGuiNodeEditor::ImGuiNodeEditor(const std::string& name, NodeFactoryFunc factoryFunc) :
    mName{name},
    mCurrentID{1},
    mNodeCreationFunction{factoryFunc},
    mContext{ax::NodeEditor::CreateEditor()}
{
}


ImGuiNodeEditor::~ImGuiNodeEditor()
{
    ax::NodeEditor::DestroyEditor(mContext);
}


void ImGuiNodeEditor::addNode(const uint64_t nodeType)
{
    std::shared_ptr<EditorNode> newNode = mNodeCreationFunction(nodeType);

    newNode->mID = mCurrentID++;

    for(auto& input : newNode->mInputs)
    {
        input.mID = mCurrentID++;
    }

    for(auto& output : newNode->mOutputs)
    {
        output.mID = mCurrentID++;
    }

    mNodes.push_back(std::move(newNode));
}


void ImGuiNodeEditor::draw()
{
    ax::NodeEditor::SetCurrentEditor(mContext);

    ax::NodeEditor::Begin(mName.c_str());

        for(const auto& node : mNodes)
        {
            ax::NodeEditor::BeginNode(node->mID);

                ImGui::TextUnformatted(node->mName.c_str());

                const uint32_t minConnections = static_cast<uint32_t>(std::min(node->mInputs.size(), node->mOutputs.size()));

                for(uint32_t i = 0; i < minConnections; ++i)
                {
                    ax::NodeEditor::BeginPin(node->mInputs[i].mID, ax::NodeEditor::PinKind::Input);

                    // We should really always have a pin name here.
                    if (!node->mInputs[i].mName.empty())
                    {
                        ImGui::TextUnformatted(node->mInputs[i].mName.c_str());
                    }

                    // TODO add some connection icon logic here.

                    ax::NodeEditor::EndPin();

                    ImGui::SameLine();

                    ax::NodeEditor::BeginPin(node->mOutputs[i].mID, ax::NodeEditor::PinKind::Output);

                    if (!node->mOutputs[i].mName.empty())
                    {
                        ImGui::TextUnformatted(node->mOutputs[i].mName.c_str());
                    }

                    // TODO add some connection icon logic here.

                    ax::NodeEditor::EndPin();
                }

                const auto& largerList = node->mInputs.size() > node->mOutputs.size() ? node->mInputs : node->mOutputs;

                for(uint32_t i = minConnections; i < largerList.size(); ++i)
                {
                    ax::NodeEditor::BeginPin(largerList[i].mID, largerList[i].mKind == PinKind::Output ? ax::NodeEditor::PinKind::Output : ax::NodeEditor::PinKind::Input);

                    if (!largerList[i].mName.empty())
                    {
                        ImGui::TextUnformatted(largerList[i].mName.c_str());
                    }

                    // TODO add some connection icon logic here.

                    ax::NodeEditor::EndPin();
                }

            ax::NodeEditor::EndNode();
        }

        for(const auto& link : mLinks)
        {
            ax::NodeEditor::Link(link.ID, link.StartPinID, link.EndPinID);
        }

    ax::NodeEditor::End();

    ax::NodeEditor::SetCurrentEditor(nullptr);
}
