#include "NodeEditor.h"

void NodeEditor::DoEditor(ed::EditorContext* ctx)
{
	
	ed::SetCurrentEditor(ctx);
	ed::Begin("My Editor", ImVec2(0.0, 0.0f));
	int uniqueId = 1;

	// Start nodes

	//Node 1
	ed::BeginNode(uniqueId++);
	ImGui::Text("Node A");
	ed::BeginPin(uniqueId++, ed::PinKind::Input);
	ImGui::Text("-> In");
	ed::EndPin();
	ImGui::SameLine();
	ed::BeginPin(uniqueId++, ed::PinKind::Output);
	ImGui::Text("Out ->");
	ed::EndPin();
	ed::EndNode();
	//Node 2
	ed::BeginNode(uniqueId++);
	ImGui::Text("Node B");
	ed::BeginPin(uniqueId++, ed::PinKind::Input);
	ImGui::Text("-> In");
	ed::EndPin();
	ImGui::SameLine();
	ed::BeginPin(uniqueId++, ed::PinKind::Output);
	ImGui::Text("Out ->");
	ed::EndPin();
	ed::EndNode();

	for (auto& link : links)
	{
		ed::Link(link.id, link.start, link.end);
	}

	if (ed::BeginCreate())
	{
		ed::PinId iPinID, oPinID;
		if (ed::QueryNewLink(&iPinID, &oPinID))
		{
			if (iPinID && oPinID)
			{
				if (ed::AcceptNewItem())
				{
					links.push_back({ ed::LinkId(nextLinkID++), iPinID, oPinID });
					ed::Link(links.back().id, links.back().start, links.back().end);
				}
			}
		}
	}
	ed::EndCreate();

	if (ed::BeginDelete())
	{
		ed::LinkId linkID;
		while (ed::QueryDeletedLink(&linkID))
		{
			
			if (ed::AcceptDeletedItem())
			{
				auto id = std::find_if(links.begin(), links.end(),
					[linkID](auto& link) {return link.id == linkID; });
				if (id != links.end())
				{
					links.erase(id);
				}
			}
		}
	}
	ed::EndDelete();


	ed::End();

	if (firstFrame)
	{
		ed::NavigateToContent(0.0f);
		firstFrame = false;
	}
	ed::SetCurrentEditor(nullptr);



	//ed::SetCurrentEditor(ctx);
	//ed::Begin("My Editor", ImVec2(0.0, 0.0f));
	//int uniqueId = 1;
	//
	//
	//ed::End();
	//
	//if (firstFrame)
	//{
	//	ed::NavigateToContent(0.0f);
	//	firstFrame = false;
	//}
	//ed::SetCurrentEditor(nullptr);
}

float NodeEditor::getTouchProgress(ed::NodeId id)
{
	auto it = nodeTouchTime.find(id);
	if (it != nodeTouchTime.end() && it->second > 0.0f)
	{
		return (touchTime - it->second) / touchTime;
	}
	return 0.0f;
}

void NodeEditor::updateTouch()
{
	const auto dT = ImGui::GetIO().DeltaTime;
	for (auto& entry : nodeTouchTime)
	{
		if (entry.second > 0.0f)
		{
			entry.second -= dT;
		}
	}
}

Node* NodeEditor::findNode(ed::NodeId id)
{
	for (auto& node : nodes)
	{
		if (node.id == id)
		{
			return &node;
		}
	}
	return nullptr;
}

Link* NodeEditor::findLink(ed::LinkId id)
{
	for (auto& link : links)
	{
		if (link.id == id)
		{
			return &link;
		}
	}
	return nullptr;
}

Pin* NodeEditor::findPin(ed::PinId id)
{
	if (!id) return nullptr;

	for (auto& node : nodes)
	{
		for (auto& pin : node.inputs)
		{
			if (pin.id == id)
			{
				return &pin;
			}
		}
		for (auto& pin : node.outputs)
		{ 
			if (pin.id == id)
			{
				return &pin;
			}
		}
	}
	return nullptr;
}

bool NodeEditor::isPinLinked(ed::PinId id)
{
	if (!id) return false;
	for (auto& link : links)
	{
		if (link.start == id || link.end == id)
		{
			return true;
		}
	}
	return false;
}

bool NodeEditor::canCreateLink(Pin* a, Pin* b)
{
	if (!a || !b || a == b || a->kind == b->kind ||
		a->type != b->type || a->node == b->node)
		return false;
	return true;
}

void NodeEditor::buildNode(Node* n)
{
	for (auto& input : n->inputs)
	{
		input.node = n;
		input.kind = PinKind::Input;
	}
	for (auto& output : n->outputs)
	{
		output.node = n;
		output.kind = PinKind::Output;
	}
}

