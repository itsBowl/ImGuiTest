#include "NodeEditor.h"

NodeEditor::NodeEditor()
{
	ed::Config config;
	config.SettingsFile = "Test.json";
	config.UserPointer = this;
	config.LoadNodeSettings = [](ed::NodeId id, char* data, void* userPtr) -> size_t
	{
		auto self = static_cast<NodeEditor*>(userPtr);
		auto node = self->findNode(id);
		if (!node) return 0;
		if (data != nullptr)
			memcpy(data, node->state.data(), node->state.size());
		return node->state.size();
	};
	config.SaveNodeSettings = [](ed::NodeId id, const char* data, size_t size,
		ed::SaveReasonFlags reason, void* userPtr)
	{
		auto self = static_cast<NodeEditor*>(userPtr);
		auto node = self->findNode(id);
		if (!node) return false;
		node->state.assign(data, size);
		self->touchNode(id);
		return true;
	};
	ctx = ed::CreateEditor(&config);
}

void NodeEditor::setup()
{
	Node* node;
	node = spawnMathNode(); ed::SetNodePosition(node->id, ImVec2(-252, 220));
	node = spawnMathNode(); ed::SetNodePosition(node->id, ImVec2(-252, 200));
	node = spawnMathNode(); ed::SetNodePosition(node->id, ImVec2(-252, 180));
	node = spawnMathNode(); ed::SetNodePosition(node->id, ImVec2(-252, 160));

	node = spawnComment(); ed::SetNodePosition(node->id, ImVec2(112, 576)); ed::SetGroupSize(node->id, ImVec2(284, 154));

	ed::NavigateToContent();

	buildNodes();
}


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


void NodeEditor::DoEditor()
{
	ed::SetCurrentEditor(ctx);
	ed::Begin("My Editor", ImVec2(0.0, 0.0f));
	if (firstFrame) { setup(); firstFrame = false; }

	ed::End();
	ed::SetCurrentEditor(nullptr);
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

void showStyleEditor(bool* show = nullptr)
{
	if (!ImGui::Begin("Style", show))
	{
		ImGui::End();
		return;
	}

	auto paneWidth = ImGui::GetContentRegionAvail().x;

	auto& editorStyle = ed::GetStyle();
	ImGui::BeginHorizontal("Style buttons", ImVec2(paneWidth, 0), 1.0f);
	ImGui::TextUnformatted("Values");
	ImGui::Spring();
	if (ImGui::Button("Reset to defaults"))
		editorStyle = ed::Style();
	ImGui::EndHorizontal();
	ImGui::Spacing();
	ImGui::DragFloat4("Node Padding", &editorStyle.NodePadding.x, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Node Rounding", &editorStyle.NodeRounding, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Node Border Width", &editorStyle.NodeBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Hovered Node Border Width", &editorStyle.HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Hovered Node Border Offset", &editorStyle.HoverNodeBorderOffset, 0.1f, -40.0f, 40.0f);
	ImGui::DragFloat("Selected Node Border Width", &editorStyle.SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Selected Node Border Offset", &editorStyle.SelectedNodeBorderOffset, 0.1f, -40.0f, 40.0f);
	ImGui::DragFloat("Pin Rounding", &editorStyle.PinRounding, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Pin Border Width", &editorStyle.PinBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Link Strength", &editorStyle.LinkStrength, 1.0f, 0.0f, 500.0f);
	//ImVec2  SourceDirection;
	//ImVec2  TargetDirection;
	ImGui::DragFloat("Scroll Duration", &editorStyle.ScrollDuration, 0.001f, 0.0f, 2.0f);
	ImGui::DragFloat("Flow Marker Distance", &editorStyle.FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
	ImGui::DragFloat("Flow Speed", &editorStyle.FlowSpeed, 1.0f, 1.0f, 2000.0f);
	ImGui::DragFloat("Flow Duration", &editorStyle.FlowDuration, 0.001f, 0.0f, 5.0f);
	//ImVec2  PivotAlignment;
	//ImVec2  PivotSize;
	//ImVec2  PivotScale;
	//float   PinCorners;
	//float   PinRadius;
	//float   PinArrowSize;
	//float   PinArrowWidth;
	ImGui::DragFloat("Group Rounding", &editorStyle.GroupRounding, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Group Border Width", &editorStyle.GroupBorderWidth, 0.1f, 0.0f, 15.0f);

	ImGui::Separator();

	static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_DisplayRGB;
	ImGui::BeginHorizontal("Color Mode", ImVec2(paneWidth, 0), 1.0f);
	ImGui::TextUnformatted("Filter Colors");
	ImGui::Spring();
	ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_DisplayRGB);
	ImGui::Spring(0);
	ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_DisplayHSV);
	ImGui::Spring(0);
	ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_DisplayHex);
	ImGui::EndHorizontal();

	static ImGuiTextFilter filter;
	filter.Draw("##filter", paneWidth);

	ImGui::Spacing();

	ImGui::PushItemWidth(-160);
	for (int i = 0; i < ed::StyleColor_Count; ++i)
	{
		auto name = ed::GetStyleColorName((ed::StyleColor)i);
		if (!filter.PassFilter(name))
			continue;

		ImGui::ColorEdit4(name, &editorStyle.Colors[i].x, edit_mode);
	}
	ImGui::PopItemWidth();

	ImGui::End();
}

void NodeEditor::showLeftPane(float width)
{
	auto& io = ImGui::GetIO();
	ImGui::BeginChild("Selection", ImVec2(width, 0));
	width = ImGui::GetContentRegionAvail().x;
	static bool ShowStyleEditor = false;

	ImGui::BeginHorizontal("Style Editor", ImVec2(width, 0));
	ImGui::Spring(0.0f, 0.0f);
	if (ImGui::Button("Zoom to Content")) ed::NavigateToContent();

	ImGui::Spring(0.0f, 0.0f);

	if (ImGui::Button("Show Flow"))
	{
		for (auto& link : links)
		{
			ed::Flow(link.id);
		}
	}

	if (ImGui::Button("Edit Style")) ShowStyleEditor = true;
	ImGui::EndHorizontal();

	ImGui::Checkbox("Show Ordinals", &showOrdinals);

	if (ShowStyleEditor) showStyleEditor(&ShowStyleEditor);

	std::vector<ed::NodeId> selectedNodes;
	std::vector<ed::LinkId> selectedLinks;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	selectedLinks.resize(ed::GetSelectedObjectCount());

	int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

	//icons
	int saveIconW = ImGui::ImGui_GetTextureWidth(saveIcon);
	int saveIconH = ImGui::ImGui_GetTextureHeight(saveIcon);
	int restoreIconW = ImGui::ImGui_GetTextureWidth(restoreIcon);
	int restoreIconH = ImGui::ImGui_GetTextureHeight(restoreIcon);

	ImGui::GetWindowDrawList()->AddRectFilled(
		ImGui::GetCursorScreenPos(),
		ImGui::GetCursorScreenPos() + ImVec2(width, ImGui::GetTextLineHeight()),
		ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
	ImGui::Spacing(); ImGui::SameLine();
	ImGui::TextUnformatted("Nodes");
	ImGui::Indent();
	for (auto& node : nodes)
	{
		ImGui::PushID(node.id.AsPointer());
		auto start = ImGui::GetCursorScreenPos();
		if (const auto progress = getTouchProgress(node.id))
		{
			ImGui::GetWindowDrawList()->AddLine(
				start + ImVec2(-8, 0),
				start + ImVec2(-8, ImGui::GetTextLineHeight()),
				IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
		}
		bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.id) != selectedNodes.end();

#if IMGUI_VERSION_NUM >= 18967
		ImGui::SetNextItemAllowOverlap();
#endif

		if (ImGui::Selectable((node.name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.id.AsPointer()))).c_str(), &isSelected))
		{
			if (io.KeyCtrl)
			{
				if (isSelected)
				{
					ed::SelectNode(node.id, true);
				}
				else
				{
					ed::DeselectNode(node.id);
				}
			}
			else
			{
				ed::SelectNode(node.id, false);
			}

			ed::NavigateToSelection();
		}

		if (ImGui::IsItemHovered() && !node.state.empty())
			ImGui::SetTooltip("State: %s", node.state.c_str());

		auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.id.AsPointer())) + ")";
		auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
		auto iconPanelPos = start + ImVec2(
			width - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconW - restoreIconW - ImGui::GetStyle().ItemInnerSpacing.x * 1,
			(ImGui::GetTextLineHeight() - saveIconH) / 2);
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
			IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

		auto drawList = ImGui::GetWindowDrawList();
		ImGui::SetCursorScreenPos(iconPanelPos);

# if IMGUI_VERSION_NUM < 18967
		ImGui::SetItemAllowOverlap();
# else
		ImGui::SetNextItemAllowOverlap();
# endif

		if (node.saveState.empty())
		{
			if (ImGui::InvisibleButton("save", ImVec2((float)saveIconW, (float)(saveIconH))))
			{
				node.saveState = node.state;

				if (ImGui::IsItemActive())
					drawList->AddImage(saveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
						ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
				else if (ImGui::IsItemHovered())
					drawList->AddImage(saveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
						ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
				else
					drawList->AddImage(saveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
						ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
			}
		}
		else
		{
			ImGui::Dummy(ImVec2((float)saveIconW, (float)saveIconH));
			drawList->AddImage(saveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
				ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
		}

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);

# if IMGUI_VERSION_NUM < 18967
		ImGui::SetItemAllowOverlap();
# else
		ImGui::SetNextItemAllowOverlap();
# endif

		if (!node.saveState.empty())
		{
			if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconW, (float)restoreIconH)))
			{
				node.state = node.saveState;
				ed::RestoreNodeState(node.id);
				node.saveState.clear();
			}

			if (ImGui::IsItemActive())
				drawList->AddImage(restoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
			else if (ImGui::IsItemHovered())
				drawList->AddImage(restoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
			else
				drawList->AddImage(restoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
					ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
		}
		else
		{
			ImGui::Dummy(ImVec2((float)restoreIconW, (float)restoreIconH));
			drawList->AddImage(restoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
				ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
		}

		ImGui::SameLine(0, 0);

# if IMGUI_VERSION_NUM < 18967
		ImGui::SetItemAllowOverlap();
# endif

		ImGui::Dummy(ImVec2(0, float(restoreIconH)));
		ImGui::PopID();
	}

	ImGui::Unindent();

	static int changeCount = 0;
	ImGui::GetWindowDrawList()->AddRectFilled(
		ImGui::GetCursorScreenPos(),
		ImGui::GetCursorScreenPos() + ImVec2(width, ImGui::GetTextLineHeight()),
		ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);

	ImGui::Spacing(); ImGui::SameLine();
	ImGui::TextUnformatted("Selection");

	ImGui::BeginHorizontal("Selection Stats", ImVec2(width, 0));
	ImGui::Text("Changed %d times", changeCount, changeCount > 1 ? "s" : "");
	ImGui::Spring();
	if (ImGui::Button("Deselect All"))
	{
		ed::ClearSelection();
	}
	ImGui::EndHorizontal();
	ImGui::Indent();
	for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
	for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
	ImGui::Unindent();

	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
	{
		for (auto& link : links)
		{
			ed::Flow(link.id);
		}
	}

	if (ed::HasSelectionChanged) ++changeCount;

	ImGui::EndChild();
}


