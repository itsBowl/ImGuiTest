#include "NodeEditor.h"
#include <imgui/imconfig.h>


namespace util = ax::NodeEditor::Utilities;



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
	int saveIconW = ImGui::getTextureWidth(saveIcon);
	int saveIconH = ImGui::getTextureHeight(saveIcon);
	int restoreIconW = ImGui::getTextureWidth(restoreIcon);
	int restoreIconH = ImGui::getTextureHeight(restoreIcon);

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

void NodeEditor::onFrame(float dt)
{
	updateTouch();

	auto& io = ImGui::GetIO();

	ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

	ed::SetCurrentEditor(ctx);

#if 0
	for (auto x = io.DisplaySize.y; x < io.DisplaySize.x; x += 10.0f)
	{
		ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y),
			IM_COL32(255, 255, 0, 255));
	}
#endif

	static ed::NodeId contextNodeID = 0;
	static ed::LinkId contextLinkID = 0;
	static ed::PinId contextPinID = 0;

	static bool createNewNode = false;
	static Pin* newNodeLinkPin = nullptr;
	static Pin* newLinkPin = nullptr;

	static float leftPaneWidth = 400.0f;
	static float rightPaneWidth = 800.0f;
	Splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f);

	showLeftPane(leftPaneWidth = 4.0f);

	ImGui::SameLine(0.0f, 12.0f);

	ed::Begin("Node Editor");
	{
		auto cursorTopLeft = ImGui::GetCursorScreenPos();

		util::BlueprintNodeBuilder builder(headerBackground, ImGui::getTextureWidth(headerBackground), ImGui::getTextureHeight(headerBackground));

		for (auto& node : nodes)
		{
			if (node.type != NodeType::Simple) continue;

			const auto isSimple = node.type == NodeType::Simple;

			bool hasOutputDelegates = false;
			for (auto& output : node.outputs)
			{
				if (output.type == PinType::Del)
				{
					hasOutputDelegates = true;
				}
			}

			builder.Begin(node.id);

			if (!isSimple)
			{
				builder.Header(node.colour);
				ImGui::Spring(0);
				ImGui::TextUnformatted(node.name.c_str());
				ImGui::Spring(1);
				ImGui::Dummy(ImVec2(0, 0));
				if (hasOutputDelegates)
				{
					ImGui::BeginVertical("delegates", ImVec2(0, 28));
					ImGui::Spring(1, 0);
					for (auto& output : node.outputs)
					{
						if (output.type != PinType::Del) continue;

						auto alpha = ImGui::GetStyle().Alpha;
						if (newLinkPin && !canCreateLink(newLinkPin, &output) && &output != newLinkPin) alpha = alpha * (48.0f / 255.0f);

						ed::BeginPin(output.id, ed::PinKind::Output);
						ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
						ed::PinPivotSize(ImVec2(0, 0));
						ImGui::BeginHorizontal(output.id.AsPointer());
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

						if (!output.name.empty())
						{
							ImGui::TextUnformatted(output.name.c_str());
							ImGui::Spring(0);
						}

						drawPinIcon(output, isPinLinked(output.id), (int)(alpha * 255));
						ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
						ImGui::EndHorizontal();
						ImGui::PopStyleVar();
						ed::EndPin();
					}

					ImGui::Spring(1, 0);
					ImGui::EndVertical();
					ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
				}
				else ImGui::Spring(0);
			}

			for (auto& input : node.inputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;

				if (newLinkPin && !canCreateLink(newLinkPin, &input) && &input != newLinkPin) alpha = alpha * (44.0f / 255.0f);

				builder.Input(input.id);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				drawPinIcon(input, isPinLinked(input.id), (int)(alpha * 255));
				ImGui::Spring(0);
				if (!input.name.empty())
				{
					ImGui::TextUnformatted(input.name.c_str());
					ImGui::Spring(0);
				}
				if (input.type == PinType::Bool)
				{
					ImGui::Button("BUTTON TEXT AT LN 675");
					ImGui::Spring(0);
				}
				ImGui::PopStyleVar();
				builder.EndInput();
			}

			if (isSimple)
			{
				builder.Middle();

				ImGui::Spring(1, 0);
				ImGui::TextUnformatted(node.name.c_str());
				ImGui::Spring(1, 0);
			}

			for (auto& output : node.outputs)
			{
				if (!isSimple && output.type == PinType::Del) continue;

				auto alpha = ImGui::GetStyle().Alpha;

				if (newLinkPin && !canCreateLink(newLinkPin, &output) && &output != newLinkPin) alpha = alpha * (48.0f / 255.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				builder.Output(output.id);
				if (output.type == PinType::String)
				{
					static char buffer[128] = "EditMeOn\nLine705!";
					static bool wasActive = false;

					ImGui::PushItemWidth(100.0f);
					ImGui::InputText("##edit", buffer, 127);
					ImGui::PopItemWidth();
					if (ImGui::IsItemActivated() && !wasActive)
					{
						ed::EnableShortcuts(false);
						wasActive = true;
					}
					else if (!ImGui::IsItemActive() && wasActive)
					{
						ed::EnableShortcuts(true);
						wasActive == false;
					}
					ImGui::Spring(0);
				}
				if (!output.name.empty())
				{
					ImGui::Spring(0);
					ImGui::TextUnformatted(output.name.c_str());
				}
				ImGui::Spring(0);
				drawPinIcon(output, isPinLinked(output.id), (int)(alpha * 255));
			}
			builder.End();
		}

		for (auto& node : nodes)
		{
			if (node.type != NodeType::Tree) continue;

			const float rounding = 5.0f;
			const float padding = 12.0f;

			const auto pinBG = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(128, 128, 128, 200));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(32, 32, 32, 200));
			ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(60, 180, 255, 150));
			ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(60, 180, 255, 150));

			ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
			ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
			ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
			ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
			ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
			ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
			ed::BeginNode(node.id);

			ImGui::BeginVertical(node.id.AsPointer());
			ImGui::BeginHorizontal("inputs");
			ImGui::Spring(0, padding * 2);

			ImRect inputsRect;
			int inputAlpha = 200;
			if (!node.inputs.empty())
			{
				auto& pin = node.inputs[0];
				ImGui::Dummy(ImVec2(0, padding));
				ImGui::Spring(1, 0);
				inputsRect = getItemRect();

				ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
				ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
#if IMGUI_VERSION_NUM > 18101
				ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersBottom);
#else
				ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
#endif
				ed::BeginPin(pin.id, ed::PinKind::Input);
				ed::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
				ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
				ed::EndPin();
				ed::PopStyleVar(3);

				if (newLinkPin && !canCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
					inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
			}
			else
				ImGui::Dummy(ImVec2(0, padding));

			ImGui::Spring(0, padding * 2);
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal("content_frame");
			ImGui::Spring(1, padding);

			ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
			ImGui::Dummy(ImVec2(160, 0));
			ImGui::Spring(1);
			ImGui::TextUnformatted(node.name.c_str());
			ImGui::Spring(1);
			ImGui::EndVertical();
			auto contentRect = getItemRect();

			ImGui::Spring(1, padding);
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal("outputs");
			ImGui::Spring(0, padding * 2);

			ImRect outputsRect;
			int outputAlpha = 200;
			if (!node.outputs.empty())
			{
				auto& pin = node.outputs[0];
				ImGui::Dummy(ImVec2(0, padding));
				ImGui::Spring(1, 0);
				outputsRect = getItemRect();
#if IMGUI_VERSION_NUM > 18101
				ed::PushStyleVar(ed::StyleVar_PinCorners, ImDrawFlags_RoundCornersTop);
#else
				ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
#endif
				ed::BeginPin(pin.id, ed::PinKind::Output);
				ed::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
				ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
				ed::EndPin();
				ed::PopStyleVar();

				if (newLinkPin && !canCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
					outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
			}
			else
				ImGui::Dummy(ImVec2(0, padding));

			ImGui::Spring(0, padding * 2);
			ImGui::EndHorizontal();

			ImGui::EndVertical();

			ed::EndNode();
			ed::PopStyleVar(7);
			ed::PopStyleColor(4);

			auto drawList = ed::GetNodeBackgroundDrawList(node.id);

#if IMGUI_VERSION_NUM > 18101
			const auto    topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
			const auto bottomRoundCornersFlags = ImDrawFlags_RoundCornersBottom;
#else
			const auto    topRoundCornersFlags = 1 | 2;
			const auto bottomRoundCornersFlags = 4 | 8;
#endif
			drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
				IM_COL32((int)(255 * pinBG.x), (int)(255 * pinBG.y), (int)(255 * pinBG.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
				IM_COL32((int)(255 * pinBG.x), (int)(255 * pinBG.y), (int)(255 * pinBG.z), inputAlpha), 4.0f, bottomRoundCornersFlags);
			//ImGui::PopStyleVar();
			drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
				IM_COL32((int)(255 * pinBG.x), (int)(255 * pinBG.y), (int)(255 * pinBG.z), outputAlpha), 4.0f, topRoundCornersFlags);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
				IM_COL32((int)(255 * pinBG.x), (int)(255 * pinBG.y), (int)(255 * pinBG.z), outputAlpha), 4.0f, topRoundCornersFlags);
			//ImGui::PopStyleVar();
			drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			drawList->AddRect(
				contentRect.GetTL(),
				contentRect.GetBR(),
				IM_COL32(48, 128, 255, 100), 0.0f);
			//ImGui::PopStyleVar();
		}

		for (auto& node : nodes)
		{
			if (node.type != NodeType::Houdini)
				continue;

			const float rounding = 10.0f;
			const float padding = 12.0f;


			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(229, 229, 229, 200));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(125, 125, 125, 200));
			ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(229, 229, 229, 60));
			ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

			const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

			ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
			ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
			ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
			ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
			ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
			ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
			ed::BeginNode(node.id);

			ImGui::BeginVertical(node.id.AsPointer());
			if (!node.inputs.empty())
			{
				ImGui::BeginHorizontal("inputs");
				ImGui::Spring(1, 0);

				ImRect inputsRect;
				int inputAlpha = 200;
				for (auto& pin : node.inputs)
				{
					ImGui::Dummy(ImVec2(padding, padding));
					inputsRect = getItemRect();
					ImGui::Spring(1, 0);
					inputsRect.Min.y -= padding;
					inputsRect.Max.y -= padding;

#if IMGUI_VERSION_NUM > 18101
					const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
#else
					const auto allRoundCornersFlags = 15;
#endif
					//ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
					//ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
					ed::PushStyleVar(ed::StyleVar_PinCorners, allRoundCornersFlags);

					ed::BeginPin(pin.id, ed::PinKind::Input);
					ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
					ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
					ed::EndPin();
					//ed::PopStyleVar(3);
					ed::PopStyleVar(1);

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);
					drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, allRoundCornersFlags);

					if (newLinkPin && !canCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
						inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
				}

				//ImGui::Spring(1, 0);
				ImGui::EndHorizontal();
			}

			ImGui::BeginHorizontal("content_frame");
			ImGui::Spring(1, padding);

			ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
			ImGui::Dummy(ImVec2(160, 0));
			ImGui::Spring(1);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::TextUnformatted(node.name.c_str());
			ImGui::PopStyleColor();
			ImGui::Spring(1);
			ImGui::EndVertical();
			auto contentRect = getItemRect();

			ImGui::Spring(1, padding);
			ImGui::EndHorizontal();

			if (!node.outputs.empty())
			{
				ImGui::BeginHorizontal("outputs");
				ImGui::Spring(1, 0);

				ImRect outputsRect;
				int outputAlpha = 200;
				for (auto& pin : node.outputs)
				{
					ImGui::Dummy(ImVec2(padding, padding));
					outputsRect = getItemRect();
					ImGui::Spring(1, 0);
					outputsRect.Min.y += padding;
					outputsRect.Max.y += padding;

#if IMGUI_VERSION_NUM > 18101
					const auto allRoundCornersFlags = ImDrawFlags_RoundCornersAll;
					const auto topRoundCornersFlags = ImDrawFlags_RoundCornersTop;
#else
					const auto allRoundCornersFlags = 15;
					const auto topRoundCornersFlags = 3;
#endif

					ed::PushStyleVar(ed::StyleVar_PinCorners, topRoundCornersFlags);
					ed::BeginPin(pin.id, ed::PinKind::Output);
					ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
					ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
					ed::EndPin();
					ed::PopStyleVar();


					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);
					drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, allRoundCornersFlags);


					if (newLinkPin && !canCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
						outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
				}

				ImGui::EndHorizontal();

				ImGui::EndVertical();
				ed::PopStyleVar(7);
				ed::PopStyleColor(4);
			}

			for (auto& node : nodes)
			{
				if (node.type != NodeType::Comment) continue;

				const float alpha = 0.75f;

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
				ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
				ed::BeginNode(node.id);
				ImGui::PushID(node.id.AsPointer());
				ImGui::BeginVertical("content");
				ImGui::BeginHorizontal("horizontal");
				ImGui::Spring(1);
				ImGui::TextUnformatted(node.name.c_str());
				ImGui::Spring(1);
				ImGui::EndHorizontal();
				ed::Group(node.size);
				ImGui::EndVertical();
				ImGui::PopID();
				ed::EndNode();
				ed::PopStyleColor(2);
				ImGui::PopStyleVar();

				if (ed::BeginGroupHint(node.id))
				{

					auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);



					auto min = ed::GetGroupMin();


					ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
					ImGui::BeginGroup();
					ImGui::TextUnformatted(node.name.c_str());
					ImGui::EndGroup();

					auto drawList = ed::GetHintBackgroundDrawList();

					auto hintBounds = getItemRect();
					auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

					drawList->AddRectFilled(
						hintFrameBounds.GetTL(),
						hintFrameBounds.GetBR(),
						IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

					drawList->AddRect(
						hintFrameBounds.GetTL(),
						hintFrameBounds.GetBR(),
						IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);


				}
				ed::EndGroupHint();
			}

			for (auto& link : links)
			{
				ed::Link(link.id, link.start, link.end, link.colour, 2.0f);
			}

			if (!createNewNode)
			{
				if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
				{
					auto showLabel = [](const char* label, ImColor colour)
					{
						ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
						auto size = ImGui::CalcTextSize(label);

						auto padding = ImGui::GetStyle().FramePadding;
						auto spacing = ImGui::GetStyle().ItemSpacing;

						ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

						auto rectMin = ImGui::GetCursorScreenPos() - padding;
						auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

						auto drawList = ImGui::GetWindowDrawList();
						drawList->AddRectFilled(rectMin, rectMax, colour, size.y * 0.15f);
						ImGui::TextUnformatted(label);
					};

					ed::PinId startID = 0, endID = 0;
					if (ed::QueryNewLink(&startID, &endID))
					{
						auto start = findPin(startID);
						auto end = findPin(endID);

						newLinkPin = start ? start : end;

						if (start->kind == PinKind::Input)
						{
							std::swap(start, end);
							std::swap(startID, endID);
						}

						if (start && end)
						{
							if (end == start) ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							else if (end->kind == start->kind)
							{
								showLabel("incompatible pin kind", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else if (end->type != start->type)
							{
								showLabel("incompatible pin type", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else
							{
								showLabel("+ create link", ImColor(32, 45, 32, 180));
								if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
								{
									links.emplace_back(Link(getNextID(), startID, endID));
									links.back().colour = getIconColor(start->type);
								}
							}
						}
					}

					ed::PinId pinID = 0;
					if (ed::QueryNewNode(&pinID))
					{
						newLinkPin = findPin(pinID);
						if (newLinkPin) showLabel("+ Create node", ImColor(32, 45, 32, 180));
						if (ed::AcceptNewItem())
						{
							createNewNode = true;
							newNodeLinkPin = findPin(pinID);
							newLinkPin = nullptr;
							ed::Suspend();
							ImGui::OpenPopup("Create New Node");
							ed::Resume();

						}
					}
				}
				else { newLinkPin = nullptr; }

				ed::EndCreate();

				if (ed::BeginDelete())
				{
					ed::NodeId nodeID = 0;
					while (ed::QueryDeletedNode(&nodeID))
					{
						if (ed::AcceptDeletedItem())
						{
							auto id = std::find_if(nodes.begin(), nodes.end(), [nodeID](auto& node) {return node.id == nodeID; });
							if (id != nodes.end()) nodes.erase(id);
						}
					}

					ed::LinkId linkID = 0;
					while (ed::QueryDeletedLink(&linkID))
					{
						if (ed::AcceptDeletedItem())
						{
							auto id = std::find_if(links.begin(), links.end(), [linkID](auto& link) { return link.id == linkID; });
							if (id != links.end()) links.erase(id);
						}
					}
				}
			}
			ed::EndDelete();
		}
		ImGui::SetCursorScreenPos(cursorTopLeft);
	}

#if 1
	auto openPopupPosition = ImGui::GetMousePos();
	ed::Suspend();
	if (ed::ShowNodeContextMenu(&contextNodeID)) ImGui::OpenPopup("Node Context Menu");
	else if (ed::ShowPinContextMenu(&contextPinID)) ImGui::OpenPopup("Pin Context Menu");
	else if (ed::ShowLinkContextMenu(&contextLinkID))ImGui::OpenPopup("Link Context Menu");
	else if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node");
		newNodeLinkPin = nullptr;
	}
	ed::Resume();
	ed::Suspend();
	if (ImGui::BeginPopup("Node Context Menu"))
	{
		auto node = findNode(contextNodeID);

		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (node)
		{
			ImGui::Text("ID: %p", node->id.AsPointer());
			ImGui::Text("Type: %s", node->type == NodeType::Tree ? "Tree" : "Comment");
			ImGui::Text("Inputs: %d", (int)node->inputs.size());
			ImGui::Text("Outputs: %d", (int)node->outputs.size());
		}
		else ImGui::Text("Unknown nod: %p", contextNodeID.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete")) ed::DeleteNode(contextNodeID);
		ImGui::EndPopup();
	}

	//WE ADD NEW NODES HERE
	if (ImGui::BeginPopup("Create New Node"))
	{
		auto newNodePos = openPopupPosition;

		Node* node = nullptr;
		if (ImGui::MenuItem("Maths")) node = spawnMathNode();
		if (ImGui::MenuItem("Comment")) node = spawnComment();

		if (node)
		{
			buildNodes();

			createNewNode = false;

			ed::SetNodePosition(node->id, newNodePos);

			if (auto startPin = newNodeLinkPin)
			{
				auto& pins = startPin->kind == PinKind::Input ? node->outputs : node->inputs;

				for (auto& pin : pins)
				{
					if (canCreateLink(startPin, &pin))
					{
						auto endPin = &pin;
						if (startPin->kind == PinKind::Input) std::swap(startPin, endPin);

						links.emplace_back(Link(getNextID(), startPin->id, endPin->id));
						links.back().colour = getIconColor(startPin->type);

						break;
					}
				}
			}
		}

		ImGui::EndPopup();
	}
	else { createNewNode = false; }
	ImGui::PopStyleVar();
	ed::Resume();
#endif

	ed::End();

	auto editorMin = ImGui::GetItemRectMin();
	auto editorMax = ImGui::GetItemRectMax();

	if (showOrdinals)
	{
		int nodeCount = ed::GetNodeCount();
		std::vector<ed::NodeId> orderedNodeID;
		orderedNodeID.resize(static_cast<size_t>(nodeCount));
		ed::GetOrderedNodeIds(orderedNodeID.data(), nodeCount);

		auto drawList = ImGui::GetWindowDrawList();
		drawList->PushClipRect(editorMin, editorMax);

		int ordinal = 0;
		for (auto& id : orderedNodeID)
		{
			auto p0 = ed::GetNodePosition(id);
			auto p1 = p0 + ed::GetNodeSize(id);

			ImGuiTextBuffer builder;
			builder.appendf("#%d", ordinal++);

			auto textSize = ImGui::CalcTextSize(builder.c_str());
			auto padding = ImVec2(2.0f, 2.0f);
			auto widgetSize = textSize + padding * 2;

			auto widgetPositoin = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -widgetSize.y);

			drawList->AddRectFilled(widgetPositoin, widgetPositoin + widgetSize, IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll);
			drawList->AddRect(widgetPositoin, widgetPositoin + widgetSize, IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll);
			drawList->AddText(widgetPositoin + padding, IM_COL32(255, 255, 255, 255), builder.c_str());
		}

		drawList->PopClipRect();
	}




}


