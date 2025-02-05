#pragma once

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

#include <GL/gl3w.h>

#include "ImGuiExtention.h"
#include <imNodeEditor/imgui_node_editor.h>
#include <imgui/imgui_internal.h>


#include <STB_Image/stb_image.h>

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
	Comment,
	Houdini
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

static inline ImRect getItemRect()
{
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
	auto r = rect;
	r.Min.x -= x;
	r.Min.y -= y;
	r.Max.x += x;
	r.Max.y += y;
	return r;
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
	
	ImColor getIconColor(PinType type)
	{
		switch (type)
		{
		default:
		//case PinType::Flow:     return ImColor(255, 255, 255);
		case PinType::Bool:     return ImColor(220, 48, 48);
		case PinType::Int:      return ImColor(68, 201, 156);
		case PinType::Float:    return ImColor(147, 226, 74);
		case PinType::String:   return ImColor(124, 21, 153);
		//case PinType::Object:   return ImColor(51, 150, 215);
		//case PinType::Function: return ImColor(218, 0, 183);
		case PinType::Del: return ImColor(255, 48, 48);
		}
	};

	ImTextureID loadTexture(const char* path)
	{
		int w = 0, h = 0, c = 0;
		if (auto data = stbi_load(path, &w, &h, &c, 4))
		{
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			
			stbi_image_free(data);

			return (ImTextureID)(intptr_t)texture;
		}
		return nullptr;
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

