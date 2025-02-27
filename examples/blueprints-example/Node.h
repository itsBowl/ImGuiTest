#pragma once


//thedmd's node definition moved from blueprints-example.cpp to here to better facilitate adding it to other files.
#include <imgui_node_editor.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <string>
#include <vector>

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

enum class PinType
{
    Flow,
    Bool,
    Int,
    Float,
    Vector,
    String,
    Object,
    Function,
    Delegate,
};

enum class PinKind
{
    Output,
    Input
};

enum class NodeType
{
    //pre-existing
    Blueprint,
    Simple,
    Tree,
    Comment,
    Houdini,
    //float types
    FloatConstant,
    FloatAdd,
    FloatSubtract,
    FloatMultiply,
    FloatDivide,
    //triganomatry
    Sin,
    Cos,
    Tan,
    //Vector
    Combine,
    Split,

    //Output
    Output
};

struct Node;

struct Pin
{
    ed::PinId   ID;
    ::Node* Node;
    std::string Name;
    PinType     Type;
    PinKind     Kind;

    Pin(int id, const char* name, PinType type) :
        ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
};

struct Node
{
    ed::NodeId ID;
    std::string Name;
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color;
    NodeType Type;
    bool isShader = false;
    ImVec2 Size;

    ImVec4 value;

    std::string State;
    std::string SavedState;

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
        ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
    {
    }
};