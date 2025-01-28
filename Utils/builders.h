#pragma once

#include <imNodeEditor/imgui_node_editor.h>

namespace ax
{
namespace NodeEditor
{
namespace Utilities
{

	struct BlueprintNodeBuilder
	{
		BlueprintNodeBuilder(ImTextureID texture = nullptr, int width = 0, int height = 0) {}

		void Begin(NodeId);
		void End();

		void Header(const ImVec4& colour = ImVec4(1, 1, 1, 1));
		void EndHeader();

		void Input(PinId);
		void EndInput();

		void Middle();

		void Output(PinId);
		void EndOutput();

	private:
		enum class Stage
		{
			Invalid,
			Begin,
			Header,
			Content,
			Input,
			Output,
			Middle,
			End
		};

		bool setStage(Stage);

		void Pin(PinId, ax::NodeEditor::PinKind);
		void EndPin();

		ImTextureID headerTexture;
		int headerTextureW, headerTextureH;
		NodeId currentNode;
		Stage currentStage;
		ImU32 headerColour;
		ImVec2 nodeMin;
		ImVec2 nodeMax;
		ImVec2 headerMin;
		ImVec2 headerMax;
		ImVec2 contentMin;
		ImVec2 contentMax;
		bool hasHeader;
			
	};
}
}
}