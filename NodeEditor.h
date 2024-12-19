#pragma once

#include <imNodeEditor/imgui_node_editor.h>


using namespace ed = ax::NodeEditor;

struct LinkInfo
{
	ed::LinkId id;
	ed::PinId inputID;
};

