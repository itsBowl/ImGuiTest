#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

#include "ImGuiExtention.h"
#include <imNodeEditor/imgui_node_editor.h>
#include <imgui/imgui_internal.h>

#include "Utils/builders.h"


namespace ed = ax::NodeEditor;

struct LinkInfo
{
	ed::LinkId id;
	ed::PinId inputID;
	ed::PinId outputID;
};

enum class PinType
{
	Bool,
	Int,
	Float,
	String,
	Obj,
	Func,
	Del
};

enum class PinKind
{
	Output,
	Input
};

enum class NodeType
{
	Simple,
	Tree,
	Comment
};

struct Node;

struct Pin
{
	ed::PinId id;
	::Node* node;
	std::string name;
	PinType type;
	PinKind kind;

	Pin(int id, const char* name, PinType type) :
		id(id), node(nullptr), name(name), type(type), kind(PinKind::Input)
	{}
};

struct Node
{
	ed::NodeId id;
	std::string name;
	std::vector<Pin> inputs;
	std::vector<Pin> outputs;

	ImColor colour;
	NodeType type;
	ImVec2 size;


	std::string state;
	std::string saveState;

	Node(int id, const char* name, ImColor colour = ImColor(255, 255, 255)) :
		id(id), name(name), colour(colour), type(NodeType::Simple), size(0, 0) 
	{}
};

struct Link
{
	ed::LinkId id;

	ed::PinId start;
	ed::PinId end;

	ImColor colour;

	Link(ed::LinkId id, ed::PinId start, ed::PinId end):
		id(id), start(start), end(end), colour(255, 255, 255)
	{}

};


struct NodeIDLess
{
	bool operator()(const ed::NodeId& lhs, const ed::NodeId rhs) const
	{
		return lhs.AsPointer() < rhs.AsPointer();
	}
};

static bool Splitter(bool v, float t, float* s1, float* s2, 
	float mS1, float mS2, float sLAS = -1.0f)
{
	//THIS IS CAUSING WEIRD ERRORS SO IT'S IN COMMENTS FOR NOW

	
	using namespace ImGui;
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiID id = window->GetID("##Splitter");
	ImRect bb;
	bb.Min = window->DC.CursorPos + (v ? ImVec2(*s1, 0.0f) : ImVec2(0.0f, *s1));
	bb.Max = bb.Min + CalcItemSize(v ? ImVec2(t, sLAS) : ImVec2(sLAS, t), 0.0f, 0.0f);
	return SplitterBehavior(bb, id, v ? ImGuiAxis_X : ImGuiAxis_Y, s1, s2, mS1, mS2, 0.0f);
	
}

class NodeEditor
{
public:

	NodeEditor();
	NodeEditor(ed::EditorContext* ctx) : ctx(ctx) {};

	void DoEditor(ed::EditorContext*);
	void DoEditor();

	int getNextID() { return nextID++; }
	ed::LinkId getNextLinkID() { return ed::LinkId(getNextID()); }
	void touchNode(ed::NodeId id) { nodeTouchTime[id] = touchTime; }
	void buildNodes() { for (auto& node : nodes) buildNode(&node); }

	float getTouchProgress(ed::NodeId);
	void updateTouch();
	Node* findNode(ed::NodeId);
	Link* findLink(ed::LinkId);
	Pin* findPin(ed::PinId);
	bool isPinLinked(ed::PinId);
	bool canCreateLink(Pin*, Pin*);
	void buildNode(Node*);
	void setup();
	ImColor getIconColour(PinType);
	void drawPinIcon(const Pin&, bool, int);
	void showStypeEditor(bool* show = nullptr); //unsure if need this
	void showLeftPane(float);
	void onFrame(float);
	


	Node* spawnMathNode()
	{
		nodes.emplace_back(getNextID(), "float math", ImColor(32, 32, 255));
		nodes.back().type = NodeType::Simple;
		nodes.back().inputs.emplace_back(getNextID(), "", PinType::Float);
		nodes.back().inputs.emplace_back(getNextID(), "", PinType::Float);
		nodes.back().outputs.emplace_back(getNextID(), "", PinType::Float);
		buildNode(&nodes.back());
		return &nodes.back();
	}

	Node* spawnComment()
	{
		nodes.emplace_back(getNextID(), "Test Comment");
		nodes.back().type = NodeType::Comment;
		nodes.back().size = ImVec2(300, 200);

		return &nodes.back();
	}
	

	ed::EditorContext* ctx;
	ImVector<LinkInfo> Imlinks;
	bool firstFrame = true;
	int nextLinkID = 100;
	int nextID = 1;
	const int pinIconSize = 24;
	std::vector<Node> nodes;
	std::vector<Link> links;
	ImTextureID headerBackground = nullptr;
	ImTextureID saveIcon = nullptr;
	ImTextureID restoreIcon = nullptr;
	const float touchTime = 1.0f;
	std::map<ed::NodeId, float, NodeIDLess> nodeTouchTime;
	bool showOrdinals = false;
};

