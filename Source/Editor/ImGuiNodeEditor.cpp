#include "ImGuiNodeEditor.h"

#include "Core/BellLogging.hpp"
#include "Engine/Engine.hpp"

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

	generateLinks(newNode);
    mNodes.push_back(std::move(newNode));
}


void ImGuiNodeEditor::addNode(std::shared_ptr<EditorNode>& newNode)
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

	generateLinks(newNode);
    mNodes.push_back(std::move(newNode));
}


void ImGuiNodeEditor::removeNode(const uint64_t nodeType)
{
    mNodes.erase(std::remove_if(mNodes.begin(), mNodes.end(), [=](const std::shared_ptr<EditorNode>& node)
    {
        return node->mType == nodeType;
    }));
}


void ImGuiNodeEditor::draw()
{
    ax::NodeEditor::SetCurrentEditor(mContext);

	ImGuiIO& io = ImGui::GetIO();

	if (io.MouseClicked[2]) // check for middle mouse click
	{
		// Handle deleting nodes/links
		std::vector<ax::NodeEditor::NodeId> nodeIds(ax::NodeEditor::GetSelectedObjectCount());
		std::vector<ax::NodeEditor::LinkId> linkIds(ax::NodeEditor::GetSelectedObjectCount());

		const auto nodeCount = ax::NodeEditor::GetSelectedNodes(nodeIds.data(), nodeIds.size());
		const auto linkCount = ax::NodeEditor::GetSelectedLinks(linkIds.data(), linkIds.size());

		nodeIds.resize(nodeCount);
		linkIds.resize(linkCount);

		for (const auto id : nodeIds)
		{
			ax::NodeEditor::DeleteNode(id);
		}

		for (const auto id : linkIds)
		{
			ax::NodeEditor::DeleteLink(id);
		}
	}

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


void ImGuiNodeEditor::addPasses(RenderEngine& eng)
{
	for (const auto& node : mNodes)
	{
		eng.registerPass(static_cast<PassType>(node->mType));
	}
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


void ImGuiNodeEditor::generateLinks(const std::shared_ptr<EditorNode>& newNode)
{
	for (const auto& node : mNodes)
	{
		for (const auto& output : node->mOutputs)
		{
			for (const auto& input : newNode->mInputs)
			{
				if (output.mName == input.mName)
				{
					mLinks.emplace_back(mCurrentID++, output.mID, input.mID);
				}
			}
		}

		for (const auto& input : node->mInputs)
		{
			for (const auto& output : newNode->mOutputs)
			{
				if (output.mName == input.mName)
				{
					mLinks.emplace_back(mCurrentID++, output.mID, input.mID);
				}
			}
		}
	}
}


std::vector<const char*> ImGuiNodeEditor::getAvailableDebugTextures() const
{
    std::vector<const char*> textures;

    for(const auto& node : mNodes)
    {
        for(const auto& output : node->mOutputs)
        {
            textures.push_back(output.mName);
        }
    }

    return textures;
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
			ax::NodeEditor::PinPivotAlignment(ImVec2(1.0f, 0.5f));
			ax::NodeEditor::PinPivotSize(ImVec2(0, 0));

            ImGui::TextUnformatted(input.mName);

			ax::NodeEditor::EndPin();
        }

        nextColumn();

        ImGui::TextUnformatted("Outputs");

        for (const auto& input : mOutputs)
        {
            ax::NodeEditor::BeginPin(input.mID, ax::NodeEditor::PinKind::Output);
			ax::NodeEditor::PinPivotAlignment(ImVec2(1.0f, 0.5f));
			ax::NodeEditor::PinPivotSize(ImVec2(0, 0));

            ImGui::TextUnformatted(input.mName);

            ax::NodeEditor::EndPin();
        }

        endColumn();

    ax::NodeEditor::EndNode();
}
