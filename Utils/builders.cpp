//#define IMGUI_DEFINE_MATH_OPERATIONS
#include "builders.h"
#include <imgui/imgui_internal.h>

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

util::BlueprintNodeBuilder::BlueprintNodeBuilder(ImTextureID texture, int width, int height) :
	headerTexture(texture), headerTextureW(width), headerTextureH(height),
	currentNode(0), currentStage(Stage::Invalid), hasHeader(false)
{}

void util::BlueprintNodeBuilder::Begin(ed::NodeId id)
{
	hasHeader = false;
	headerMin = headerMax = ImVec2();

	ed::PushStyleVar(StyleVar_NodePadding, ImVec4(8, 4, 8, 8));

	ed::BeginNode(id);

	ImGui::PushID(id.AsPointer());
	currentNode = id;
	setStage(Stage::Begin);
}

void util::BlueprintNodeBuilder::End()
{
	setStage(Stage::End);

	ed::EndNode();

	if (ImGui::IsItemVisible())
	{
		auto alpha = static_cast<int>(255 * ImGui::GetStyle().Alpha);

		auto drawList = ed::GetNodeBackgroundDrawList(currentNode);

		const auto halfBoarderWidth = ed::GetStyle().NodeBorderWidth * 0.5f;

		auto headercolour = IM_COL32(0, 0, 0, alpha) | (headerColour & IM_COL32(255, 255, 255, 0));
		if ((headerMax.x > headerMin.x && headerMax.y > headerMin.y) && headerTexture)
		{
			const auto uv = ImVec2(
				(headerMax.x - headerMin.x) / float(4.0f * headerTextureW),
				(headerMax.y - headerMin.y) / float(4.0f * headerTextureH)
			);

			drawList->AddImageRounded(headerTexture,
				headerMin - ImVec2(8 - halfBoarderWidth, 4 - halfBoarderWidth),
				headerMax - ImVec2(8 - halfBoarderWidth, 0),
				ImVec2(0.0f, 0.0f), uv,
#if IMGUI_VERSION_NUM > 18101
				headercolour, GetStyle().NodeRounding, ImDrawFlags_RoundCornersTop);
#else
				headercolour, GetStyle().NodeRounding, 1 | 2);
#endif

			if (contentMin.y > headerMax.y)
			{
				drawList->AddLine(
					ImVec2(headerMin.x - (8 - halfBoarderWidth), headerMax.y - 0.5f),
					ImVec2(headerMax.x - (8 - halfBoarderWidth), headerMax.y - 0.5f),
					ImColor(255, 255, 255, 96 * alpha / (4 * 255)), 1.0f);
			}

			currentNode = 0;

			ImGui::PopID();
			ed::PopStyleVar();
			setStage(Stage::Invalid);
		}

		
	}
}

void util::BlueprintNodeBuilder::Header(const ImVec4& colour)
{
	headerColour = ImColor(colour);
	setStage(Stage::Header);
}

void util::BlueprintNodeBuilder::EndHeader()
{
	setStage(Stage::Content);
}

void util::BlueprintNodeBuilder::Input(ed::PinId id)
{
	if (currentStage == Stage::Begin) setStage(Stage::Content);

	const auto applyPadding = (currentStage == Stage::Input);

	setStage(Stage::Input);
	if (applyPadding) ImGui::Spring(0);

	Pin(id, PinKind::Input);
	
	ImGui::BeginHorizontal(id.AsPointer());
}

void util::BlueprintNodeBuilder::EndInput()
{
	ImGui::EndHorizontal();
	EndPin();
}

void util::BlueprintNodeBuilder::Middle()
{
	if (currentStage == Stage::Begin) setStage(Stage::Content);

	setStage(Stage::Middle);
}

void util::BlueprintNodeBuilder::Output(ed::PinId id)
{
	if (currentStage == Stage::Begin) setStage(Stage::Content);

	const auto applyPadding = (currentStage == Stage::Output);

	setStage(Stage::Output);

	if (applyPadding) ImGui::Spring(0);

	Pin(id, PinKind::Output);
	ImGui::BeginHorizontal(id.AsPointer());
}

void util::BlueprintNodeBuilder::EndOutput()
{
	ImGui::EndHorizontal();
	EndPin();
}

bool util::BlueprintNodeBuilder::setStage(Stage stage)
{
	if (stage == currentStage) return false;

	auto oldStage = currentStage;

	currentStage = stage;
	ImVec2 cursor;
	switch (oldStage)
	{
	case Stage::Begin:
		break;
	case Stage::Header:
		ImGui::EndHorizontal();
		headerMin = ImGui::GetItemRectMin();
		headerMax = ImGui::GetItemRectMax();

		ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.y * 2.0f);
		break;
	case Stage::Content:
		break;
	case Stage::Input:
		ed::PopStyleVar(2);

		ImGui::Spring(1, 0);
		ImGui::EndVertical();
		break;
	case Stage::Middle:
		ImGui::EndVertical();
		break;
	case Stage::Output:
		ed::PopStyleVar(2);
		ImGui::Spring(1, 0);
		ImGui::EndVertical();
		break;
	case Stage::End:
		break;
	case Stage::Invalid:
		break;
	}

	switch (stage)
	{
	case Stage::Begin:
		ImGui::BeginVertical("node");
		break;
	case Stage::Header:
		hasHeader = true;
		ImGui::BeginHorizontal("header");
		break;
	case Stage::Content:
		if (oldStage == Stage::Begin) ImGui::Spring(0);

		ImGui::BeginHorizontal("content");
		ImGui::Spring(0, 0);
		break;
	case Stage::Input:
		ImGui::BeginVertical("inputs", ImVec2(0, 0), 0.0f);
		
		ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(0, 0.5f));
		ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));

		if (!hasHeader) ImGui::Spring(1, 0);

		break;
	case Stage::Middle:
		ImGui::Spring(1);
		ImGui::BeginVertical("middle", ImVec2(0, 0), 1.0f);
		break;
	case Stage::Output:
		if (oldStage == Stage::Middle || oldStage == Stage::Input) ImGui::Spring(1);
		else ImGui::Spring(1, 0);

		ImGui::BeginVertical("outputs", ImVec2(0, 0), 1.0f);

		ed::PushStyleVar(ed::StyleVar_PivotAlignment, ImVec2(1.0f, 0.5f));
		ed::PushStyleVar(ed::StyleVar_PivotSize, ImVec2(0, 0));
		
		if (!hasHeader) ImGui::Spring(1, 0);
		
		break;
	case Stage::End:
		if (oldStage == Stage::Input) ImGui::Spring(1, 0);
		if (oldStage != Stage::Begin) ImGui::EndHorizontal();
		contentMin = ImGui::GetItemRectMin();
		contentMax = ImGui::GetItemRectMax();

		ImGui::EndVertical();
		nodeMin = ImGui::GetItemRectMin();
		nodeMax = ImGui::GetItemRectMax();
		break;
	case Stage::Invalid:
		break;
	}

	return true;
}

void util::BlueprintNodeBuilder::Pin(ed::PinId id, ed::PinKind kind)
{
	ed::BeginPin(id, kind);
}

void util::BlueprintNodeBuilder::EndPin()
{
	ed::EndPin();
}