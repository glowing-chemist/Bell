#include "ImGuiNodeEditor.h"
#include "Core/BellLogging.hpp"

#include <algorithm>

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


void ImGuiNodeEditor::addNode(const std::shared_ptr<EditorNode>& newNode)
{
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
           node->draw();
        }

		if (ax::NodeEditor::BeginCreate())
		{
            ax::NodeEditor::PinId startPinID, endPinID;
            if (ax::NodeEditor::QueryNewLink(&startPinID, &endPinID))
			{

                const Pin& inputPin = findPin(startPinID);
                const Pin& outputPin = findPin(endPinID);

                if (startPinID && endPinID && (inputPin.mKind != outputPin.mKind))
				{
					if (ax::NodeEditor::AcceptNewItem())
					{

                        mLinks.push_back({ ax::NodeEditor::LinkId(mCurrentID++), startPinID, endPinID });

						// Draw new link.
						ax::NodeEditor::Link(mLinks.back().mID, mLinks.back().mStartPinID, mLinks.back().mEndPinID);
					}
				}
				else
					ax::NodeEditor::RejectNewItem();
			}
		}

		ax::NodeEditor::EndCreate();

		if (ax::NodeEditor::BeginDelete())
		{
			// There may be many links marked for deletion, let's loop over them.
			ax::NodeEditor::LinkId deletedLinkId;
			while (ax::NodeEditor::QueryDeletedLink(&deletedLinkId))
			{
				// TODO Add deletion logic.
				if (ax::NodeEditor::AcceptDeletedItem())
				{
					mLinks.erase(std::remove_if(mLinks.begin(), mLinks.end(), [deletedLinkId](const auto& link)
					{
						return link.mID == deletedLinkId;
					}), mLinks.end());
				}

				// You may reject link deletion by calling:
				// ed::RejectDeletedItem();
			}
		}

		ax::NodeEditor::EndDelete();

        for(const auto& link : mLinks)
        {
            ax::NodeEditor::Link(link.mID, link.mStartPinID, link.mEndPinID);
        }

    ax::NodeEditor::End();

    ax::NodeEditor::SetCurrentEditor(nullptr);
}


const Pin& ImGuiNodeEditor::findPin(const ax::NodeEditor::PinId pinID)
{
    for(const auto& node : mNodes)
    {
        if(const auto pin = std::find_if(node->mInputs.begin(), node->mInputs.end(), [pinID] ( const Pin& inPin )
        {
            return inPin.mID == pinID;
        }); pin != node->mInputs.end())
        {
            return *pin;
        }

        if(const auto pin = std::find_if(node->mOutputs.begin(), node->mOutputs.end(), [pinID] ( const Pin& inPin )
        {
            return inPin.mID == pinID;
        }); pin != node->mOutputs.end())
        {
            return *pin;
        }
    }

    // This should be unreachable.
    BELL_TRAP;
}


void PassNode::draw()
{
    ax::NodeEditor::BeginNode(mID);

        ImGui::TextUnformatted(mName.c_str());

        beginColumn();

        ImGui::TextUnformatted("Inputs");

        for (const auto& input : mInputs)
        {
            ax::NodeEditor::BeginPin(input.mID, ax::NodeEditor::PinKind::Input);

            ImGui::TextUnformatted(input.mName.c_str());

            ax::NodeEditor::EndPin();
        }

        nextColumn();

        ImGui::TextUnformatted("Outputs");

        for (const auto& input : mOutputs)
        {
            ax::NodeEditor::BeginPin(input.mID, ax::NodeEditor::PinKind::Output);

            ImGui::TextUnformatted(input.mName.c_str());

            ax::NodeEditor::EndPin();
        }

        endColumn();

    ax::NodeEditor::EndNode();
}


void ResourceNode::draw()
{
    ax::NodeEditor::BeginNode(mID);

        //ImGui::InputText("Resource Name", mName.data(), mName.size());

        beginColumn();

        for (const auto& input : mInputs)
        {
            ax::NodeEditor::BeginPin(input.mID, ax::NodeEditor::PinKind::Input);

            ImGui::TextUnformatted(input.mName.c_str());

            ax::NodeEditor::EndPin();
        }

        nextColumn();

        for (const auto& input : mOutputs)
        {
            ax::NodeEditor::BeginPin(input.mID, ax::NodeEditor::PinKind::Output);

            ImGui::TextUnformatted(input.mName.c_str());

            ax::NodeEditor::EndPin();
        }

        endColumn();

    ax::NodeEditor::EndNode();
}
