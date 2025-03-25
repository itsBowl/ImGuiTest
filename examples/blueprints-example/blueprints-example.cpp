#define IMGUI_DEFINE_MATH_OPERATORS
#include <application.h>
#include "utilities/builders.h"
#include "utilities/widgets.h"

#include <imgui_node_editor.h>
#include <imgui_internal.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <utility>

#include <iostream>
#include <unordered_map>

#include "Extentions.h"
#include "Render.h"
#include "Program.h"
#include "Nodes.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "ExtraShaderCode.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <algorithm>


static inline ImRect ImGui_GetItemRect()
{
    return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
    auto result = rect;
    result.Min.x -= x;
    result.Min.y -= y;
    result.Max.x += x;
    result.Max.y += y;
    return result;
}

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

using namespace ax;
using json = nlohmann::json;

using ax::Widgets::IconType;

static ed::EditorContext* m_Editor = nullptr;

//extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vkey);
//extern "C" bool Debug_KeyPress(int vkey)
//{
//    static std::map<int, bool> state;
//    auto lastState = state[vkey];
//    state[vkey] = (GetAsyncKeyState(vkey) & 0x8000) != 0;
//    if (state[vkey] && !lastState)
//        return true;
//    else
//        return false;
//}



struct Node;

struct Pin
{
    ed::PinId   ID;
    ::Node*     Node;
    std::string Name;
    PinType     Type;
    PinKind     Kind;

    Pin(int id, const char* name, PinType type):
        ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
    {
    }
};

struct Node
{
    ed::NodeId ID;
    std::string Name;
    std::string userDefinedName = "";
    std::vector<Pin> Inputs;
    std::vector<Pin> Outputs;
    ImColor Color;
    NodeType Type;
    bool isShader = false;
    ImVec2 Size;
    int lineNumber = -1;

    ImVec4 value;
    uint64_t index;

    std::string State;
    std::string SavedState;

    Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)):
        ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
    {
    }
};


struct Link
{
    ed::LinkId ID;

    ed::PinId StartPinID;
    ed::PinId EndPinID;

    uint64_t startNodeIdx;
    uint64_t startPinOffset;
    uint64_t endNodeIdx;
    uint64_t endPinOffset;
    

    ImColor Color;

    Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId):
        ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
    {
    }
};

struct NodeIdLess
{
    bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
    {
        return lhs.AsPointer() < rhs.AsPointer();
    }
};

static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

struct Example:
    public Application
{
    using Application::Application;

    std::string displayShaderCode = "";
    std::string shaderCode = "";

    int GetNextId()
    {
        return m_NextId++;
    }

    //ed::NodeId GetNextNodeId()
    //{
    //    return ed::NodeId(GetNextId());
    //}

    ed::LinkId GetNextLinkId()
    {
        return ed::LinkId(GetNextId());
    }

    void TouchNode(ed::NodeId id)
    {
        m_NodeTouchTime[id] = m_TouchTime;
    }

    float GetTouchProgress(ed::NodeId id)
    {
        auto it = m_NodeTouchTime.find(id);
        if (it != m_NodeTouchTime.end() && it->second > 0.0f)
            return (m_TouchTime - it->second) / m_TouchTime;
        else
            return 0.0f;
    }

    void UpdateTouch()
    {
        const auto deltaTime = ImGui::GetIO().DeltaTime;
        for (auto& entry : m_NodeTouchTime)
        {
            if (entry.second > 0.0f)
                entry.second -= deltaTime;
        }
    }

    Node* findConnected(ed::PinId pin)
    {
        for (auto& l : m_Links)
        {
            if (l.EndPinID == pin)
            {
                for (auto& n : m_Nodes)
                {
                    for (auto o : n.Outputs)
                    {
                        if (o.ID == l.StartPinID)
                        {
                            return &n;
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    void to_json(json& j, const std::vector<Node>& nodes)
    {
        j = json::array();
        for (auto& n : nodes)
        {
            json node;
            to_json(node, n);
            j.push_back(node);
        }
    }
    void to_json(json& j, const Node& n)
    {
        j = json{
            { "index", n.index },
            { "name", n.Name },
            { "userName", n.userDefinedName },
            { "type", static_cast<uint32_t>(n.Type)},
            {"x", n.value.x},
            {"y", n.value.y},
            {"z", n.value.z},
            {"w", n.value.w},
            {"state", n.State}
        };
    }
    void from_json(json& j, Node& n)
    {
        j.at("index").get_to(n.index);
        j.at("name").get_to(n.Name);
        j.at("userName").get_to(n.userDefinedName);
        j.at("type").get_to(n.Type);
        j.at("x").get_to(n.value.x);
        j.at("y").get_to(n.value.y);
        j.at("z").get_to(n.value.z);
        j.at("w").get_to(n.value.w);
    }
    void from_json(json& j, Node* n)
    {
        j.at("index").get_to(n->index);
        j.at("name").get_to(n->Name);
        j.at("userName").get_to(n->userDefinedName);
        j.at("type").get_to(n->Type);
        j.at("x").get_to(n->value.x);
        j.at("y").get_to(n->value.y);
        j.at("z").get_to(n->value.z);
        j.at("w").get_to(n->value.w);
        j.at("state").get_to(n->SavedState);
    }

    void to_json(json& j, const Link& l)
    {
        j = json{
            {"startNodeIdx", l.startNodeIdx},
            {"startPinOffset", l.startPinOffset},
            {"endNodeIdx", l.endNodeIdx},
            {"endPinOffset", l.endPinOffset},
        };
    }

    void to_json(json& j, const std::vector<Link>& links)
    {
        j = json::array();
        for (auto& l : links)
        {
            json link;
            to_json(link, l);
            j.push_back(link);
        }
    }
    
    void displayCode(int ln, ImVec4 col = ImVec4(1, 1, 1, 1))
    {
        for (size_t i = 0; i < codeAsLines.size(); i++)
        {
            if (i == codeAsLines.size() - 1) break;
            if (i == ln)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, col);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, col);
                ImGui::Text( "%i:   %s", i + 1, codeAsLines[i].c_str());
                ImGui::PopStyleColor(2);
                //ImGui::NewLine();

            }
            else
            {
                ImGui::Text("%i:    %s", i + 1, codeAsLines[i].c_str());
            }
        }
    }

    std::string getNodeTooltip(Node n)
    {
        return getTooltip(n.Type);
    }

    void drawTooltip(Node& n)
    {
        
        ImVec2 mPos = ed::CanvasToScreen(ImGui::GetIO().MousePos);
        ImVec2 finalPos = ImVec2(mPos.x + 10.f, mPos.y + 10.f);
        //finalPos.x = ImClamp(finalPos.x, 1.0f, ImGui::GetIO().DisplaySize.x - 10.0f);
        //finalPos.y = ImClamp(finalPos.y, 1.0f, ImGui::GetIO().DisplaySize.y - 10.0f);
        //std::cout << finalPos.x << " " << finalPos.y << std::endl;
        ImGui::SetNextWindowPos(finalPos); 
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 10);
        ImGui::TextUnformatted(getNodeTooltip(n).c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }

    void displayCode(std::vector<int> selected, int errLn = -1)
    {
        for (size_t i = 0; i < codeAsLines.size(); i++)
        {
            if (i == codeAsLines.size() - 1) break;
            if (std::find(selected.begin(), selected.end(), i) != selected.end())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 1, 1));
                //ImGui::PushStyleColor(ImGuiCol_FrameBg, col);
                ImGui::TextWrapped("%i:\t\t%s", i + 1, codeAsLines[i].c_str());
                ImGui::PopStyleColor(1);
                //ImGui::NewLine();

            }
            else if (i == errLn)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1));
                //ImGui::PushStyleColor(ImGuiCol_FrameBg, col);
                ImGui::TextWrapped("%i:\t\t%s", i + 1, codeAsLines[i].c_str());
                ImGui::PopStyleColor(1);
            }
            else
            {
                ImGui::TextWrapped("%i:\t\t%s", i + 1, codeAsLines[i].c_str());
            }
        }
    }



    void displayCodeWithSelected(int ln)
    {
        displayCode(ln, ImVec4(1, 0, 1, 1));
    }

    void displayCodeWithError(int ln)
    {
        displayCode(ln, ImVec4(1, 0, 0, 1));
    }
    void from_json(json& j, Link& l)
    {
        j.at("startNodeIdx").get_to(l.startNodeIdx);
        j.at("startPinOffset").get_to(l.startPinOffset);
        j.at("endNodeIdx").get_to(l.endNodeIdx);
        j.at("endPinOffset").get_to(l.endPinOffset);
    }

    std::string serialiseNode(Node* n)
    {
        
    }

    //Very WIP needs significant work
    void copyNodes(json& j)
    {
        std::cout << "Copied Nodes: \n";
        int selection = ed::GetSelectedObjectCount();
        //testing

        //selection = m_Nodes.size();
        if (selection <= 0) return;

        std::vector<ed::NodeId> selected(selection);
        ed::GetSelectedNodes(selected.data(), selection);        
        
        std::unordered_map<uint64_t, uint64_t> indexMap;
        
        copiedNodes.clear();
        copiedLinks.clear();
        for (size_t i = 0; i < selected.size(); i++)
        {
            uint64_t id = selected[i].Get();
            for (auto& n : m_Nodes)
            {
                if (n.ID.Get() == id)
                {
                    indexMap[id] = copiedNodes.size();
                    n.index = indexMap[id];
                    copiedNodes.push_back(n);
                }
            }
        }

        serialiseLinks(indexMap);
        std::cout << "End of copy operation\n";
    }

    void serialiseNodeTree()
    {
        copiedNodes.clear();
        copiedLinks.clear();
        std::unordered_map<uint64_t, uint64_t> indexMap;

        for (size_t i = 0; i < m_Nodes.size(); i++)
        {
            uint64_t id = m_Nodes[i].ID.Get();
            for (auto& n : m_Nodes)
            {
                if (n.ID.Get() == id)
                {
                    indexMap[id] = copiedNodes.size();
                    n.index = indexMap[id];
                    copiedNodes.push_back(n);
                }
            }
        }
        serialiseLinks(indexMap);

        outputJson["nodes"] = json::array();
        for (auto& n : copiedNodes)
        {
            json node;
            to_json(node, n);
            outputJson["nodes"].push_back(node);
        }
        outputJson["links"] = json::array();
        for (auto& l : copiedLinks)
        {
            json link;
            to_json(link, l);
            outputJson["links"].push_back(link);
        }
        std::cout << "End of serialise operation\n";
    }

    void serialiseLinks(std::unordered_map<uint64_t, uint64_t>& indexMap)
    {
        for (const auto& l : m_Links)
        {
            uint64_t startPin = static_cast<uint64_t>(l.StartPinID.Get());
            uint64_t endPin = static_cast<uint64_t>(l.EndPinID.Get());
            uint64_t startNodeId;
            uint64_t endNodeId;

            for (auto& n : copiedNodes)
            {
                for (auto& p : n.Inputs)
                {
                    if (p.ID == l.EndPinID)
                    {
                        endNodeId = static_cast<uint64_t>(n.ID.Get());
                    }
                }
                for (auto& p : n.Outputs)
                {
                    if (p.ID == l.StartPinID)
                    {
                        startNodeId = static_cast<uint64_t>(n.ID.Get());
                    }
                }

            }
            if (indexMap.count(startNodeId) && indexMap.count(endNodeId))
            {
                Link copied(0, 0, 0);
                copied.startNodeIdx = indexMap[startNodeId];
                copied.endNodeIdx = indexMap[endNodeId];
                copied.startPinOffset = l.StartPinID.Get() - startNodeId;
                copied.endPinOffset = l.EndPinID.Get() - endNodeId;
                copiedLinks.push_back(copied);
            }
        }
    }

    void pasteNodes()
    {
        std::vector<ed::NodeId> newNodes;
        for (auto n : copiedNodes)
        {
            Node* newNode = nullptr;
            switch (n.Type)
            {
            case NodeType::FloatConstant:
                newNode = spawnFloatConstantNode();
                newNode->value.x = n.value.x;
            }
        }
    }

    std::string getNodeName(Node& node)
    {
        if (node.userDefinedName != "")
        {
            return node.userDefinedName;
        }
        else
        {
            return "var" + std::to_string(node.ID.Get());
        }
    }

    void getVariableNameFromSplit(Node* connected, Node* current, std::vector<std::string>& vars, Pin& p)
    {
        if (connected->Type == NodeType::UV)
        {
            for (auto& l : m_Links)
            {
                for (auto& o : connected->Outputs)
                {
                    if (l.EndPinID == p.ID && l.StartPinID == o.ID)
                    {
                        if (o.Name == "x")
                        {
                            vars.push_back("vUV.x");
                        }
                        else if (o.Name == "y")
                        {
                            vars.push_back("vUV.y");
                        }
                    }
                }
            }
        }
        else if (connected->Type == NodeType::Split)
        {
            Node* prev;
            //split is garenteed to have one input, but we're doing this because it's safer
            for (auto& i : connected->Inputs)
            {
                prev = findConnected(i.ID);
            }
            for (auto& l : m_Links)
            {
                for (auto& o : connected->Outputs)
                {
                    if (l.EndPinID == p.ID && l.StartPinID == o.ID)
                    {
                        if (o.Name == "x")
                        {
                            vars.push_back(getNodeName(*prev) + ".x");
                        }
                        else if (o.Name == "y")
                        {
                            vars.push_back(getNodeName(*prev) + ".y");
                        }
                        else if (o.Name == "z")
                        {
                            vars.push_back(getNodeName(*prev) + ".z");
                        }
                        else if (o.Name == "w")
                        {
                            vars.push_back(getNodeName(*prev) + ".w");
                        }
                    }
                }
            }
        }
    }

    std::string generateNodeCodeStr(Node& node, std::unordered_map<uint64_t, std::string>& variableNames)
    {
        std::string code = "";
        std::vector<std::string> inVars;

        for (auto& p : node.Inputs)
        {
            Node* connected = findConnected(p.ID);
            if (connected)
            {
                if (variableNames.count(connected->ID.Get()) == 0)
                {
                    code += generateNodeCodeStr(*connected, variableNames);
                }
                //slightly hacky system for pushing UV input variables into the inVars structure without redefining them
                if (connected->Type == NodeType::UV || connected->Type == NodeType::Split)
                {
                    getVariableNameFromSplit(connected, &node, inVars, p);
                }
                else
                {
                    inVars.push_back(variableNames[connected->ID.Get()]);
                }
            }
            else
            {
                inVars.push_back("0.0f");
            }
        }
        std::string nodeVar = "";
        if (node.userDefinedName != "")
        {
            nodeVar = node.userDefinedName;
        }
        else
        {
            nodeVar = "var" + std::to_string(node.ID.Get());
        }
        if (node.Type == NodeType::Output)
        {
            nodeVar = "fragColour ";
        }
        else if (node.Type == NodeType::Time)
        {
            nodeVar = "time ";
        }
        else if (node.Type == NodeType::FloatConstant)
        {

            nodeVar = std::to_string(node.value.x - (long)node.value.x);
            int sigFigs = 0;
            int size = nodeVar.size();

            for (auto i = size - 1; i > 0; i--)
            {
                char c = nodeVar[i];
                nodeVar.pop_back();
                int iC = c - '0';
                if (iC != 0)
                {
                    if (i == 1)
                    {
                        sigFigs = i;
                        break;
                    }
                    sigFigs = i - 1;
                    break;
                }
            }

            nodeVar = std::format("{:.{}f}f", node.value.x, sigFigs);
            //std::cout << "Node :" << node.userDefinedName << " Value: " << nodeVar << "  " << sigFigs << "\n";
        }
        variableNames[node.ID.Get()] = nodeVar;

        switch (node.Type)
        {
        case NodeType::FloatConstant:
            //since we know the value here is constant we can read it directly from the node and not worry about pin linking ;p
            break;
        case NodeType::FloatAdd:
            code += "float " + nodeVar + " = " + inVars[0] + " + " + inVars[1] + ";\n";
            break;
        case NodeType::FloatSubtract:
            code += "float " + nodeVar + " = " + inVars[0] + " - " + inVars[1] + ";\n";
            break;
        case NodeType::FloatMultiply:
            code += "float " + nodeVar + " = " + inVars[0] + " * " + inVars[1] + ";\n";
            break;
        case NodeType::FloatDivide:
            code += "float " + nodeVar + " = " + inVars[0] + " / " + inVars[1] + ";\n";
            break;
        case NodeType::FloatPow:
            code += "float " + nodeVar + " = pow(" + inVars[0] + ", " + inVars[1] + ");\n";
            break;
        case NodeType::FloatAbsolute:
            code += "float " + nodeVar + " = abs(" + inVars[0] + ");\n";
            break;
        case NodeType::Sign:
            code += "float " + nodeVar + " = sign(" + inVars[0] + ");\n";
            break;
        case NodeType::Floor:
            code += "float " + nodeVar + " = floor(" + inVars[0] + ");\n";
            break;
        case NodeType::Ceil:
            code += "float " + nodeVar + " = ceil(" + inVars[0] + ");\n";
            break;
        case NodeType::Fract:
            code += "float " + nodeVar + " = fract(" + inVars[0] + ");\n";
            break;
        case NodeType::Mod:
            code += "float " + nodeVar + " = mod(" + inVars[0] + ", " + inVars[1] + ");\n";
            break;
        case NodeType::FloatMin:
            code += "float " + nodeVar + " = min(" + inVars[0] + ", " + inVars[1] +  ");\n";
            break;
        case NodeType::FloatMax:
            code += "float " + nodeVar + " = max(" + inVars[0] + ", " + inVars[1] + ");\n";
            break;
        case NodeType::Clamp:
            code += "float " + nodeVar + " = clamp(" + inVars[0] + ", " + inVars[1] + ", " + inVars[2] + ");\n";
            break;
        case NodeType::Mix:
            code += "float " + nodeVar + " = mix(" + inVars[0] + ", " + inVars[1] + ", " + inVars[2] + "0;\n";

        //Triganometry
        case NodeType::Sin:
            code += "float " + nodeVar + " = sin(" + inVars[0] + ");\n";
            break;
        case NodeType::Cos:
            code += "float " + nodeVar + " = cos(" + inVars[0] + ");\n";
            break;
        case NodeType::Tan:
            code += "float " + nodeVar + " = tan(" + inVars[0] + ");\n";
            break;
        //Vector
        // causing errors with drawing nodes, will fix if have time
        //case NodeType::VectorConstant:
        //    code += "vec4 " + nodeVar + " = " + "vec4(" + std::to_string(node.value.x) + ", " + std::to_string(node.value.y) + ", " 
        //        + std::to_string(node.value.z) + ", " + std::to_string(node.value.w) + ");\n";
        case NodeType::VectorAdd:
            code += "vec4 " + nodeVar + " = " + inVars[0] + " + " + inVars[1] + ";\n";
            break;
        case NodeType::VectorSubtract:
            code += "vec4 " + nodeVar + " = " + inVars[0] + " - " + inVars[1] + ";\n";
            break;
        case NodeType::VectorMultiply:
            code += "vec4 " + nodeVar + " = " + inVars[0] + " * " + inVars[1] + ";\n";
            break;
        case NodeType::VectorDivide:
            code += "vec4 " + nodeVar + " = " + inVars[0] + " / " + inVars[1] + ";\n";
            break;
        case NodeType::VectorAbs:
            code += "vec4 " + nodeVar + " = abs(" + inVars[0] + ");\n";
            break;
        case NodeType::Dot:
            code += "float " + nodeVar + " = dot(", inVars[0] + ", " + inVars[1] + ");\n";
            break;
        case NodeType::Cross:
            code += "vec4 " + nodeVar + " = cross(", inVars[0] + ", " + inVars[1], ");\n";
            break;
        case NodeType::Length:
            code += "float " + nodeVar + " = length("+ inVars[0] + ");\n";
            break;
        case NodeType::Normalize:
            code += "vec4 " + nodeVar + " = normalize(" + inVars[0] + ");\n";
            break;

        //Vector Utilities
        case NodeType::Combine:
            code += "vec4 " + nodeVar + " = vec4(" + inVars[0] + ", " + inVars[1] + ", " + inVars[2] + ", " + inVars[3] + ");\n";
            break;
        //these nodes don't add any code, so we just stack them up here
        case NodeType::Split:
        case NodeType::UV:
        case NodeType::Time:
            break;

        //float perlinNoise(vec2 position, int frequency, int octaveCount, float persistence, float lacunarity, uint seed)
        case NodeType::PerlinNoise:
            code += "float " + nodeVar + " = perlinNoise(vec2(" + inVars[0] + "), "
                + inVars[1] + ", " + inVars[2] + ", " + inVars[3] +
                ", " + inVars[4] + ", " + inVars[5] + ");\n";
                break;
        case NodeType::Output:
            code += nodeVar + " = vec4(" + inVars[0] + ");\n";
            break;
        case NodeType::SimpleNoise:
            code += "float " + nodeVar + " = simpleNoise(vec2(" + inVars[0] + "), "
                + inVars[1] + ", " + inVars[2] +", " + inVars[3] + "); \n";
            break;
        }

        if (node.Type == NodeType::UV)
        {
            node.lineNumber = 1;
        }
        else if (node.Type == NodeType::Time)
        {
            node.lineNumber = 2;
        }
        else if (node.Type == NodeType::FloatConstant || node.Type == NodeType::Time)
        {
        }
        else
        {
            node.lineNumber = currentLineNumber;
            currentLineNumber++;
        }
        

        //generated[node.ID.Get()] = code;

        return code;
    }

    void buildShader()
    {
        currentLineNumber = 6;
        codeAsLines.clear();
        displayShaderCode.clear();
        shaderCode.clear();
        program_one.errLog.clear();
        program_two.errLog.clear();
        std::unordered_map<uint64_t, std::string> names;
        std::unordered_map<uint64_t, std::string> code;
        shaderCode = "#version 450 core\nin vec2 vUV;\nin float time;\nlayout (location = 0) out vec4 fragColour;";
        shaderCode += perlinShaderCode;
            
        shaderCode += "\n\nvoid main() {\n";
        displayShaderCode = "#version 450 core\nin vec2 vUV;\nin float time;\nlayout (location = 0) out vec4 fragColour;\n\nvoid main() {\n";
        std::string generatedShaderCode = "";

        for (auto& n : m_Nodes)
        {
            if (n.Type == NodeType::Output)
            {
                generatedShaderCode += generateNodeCodeStr(n, names);
                //generateNodeCode(n, names, code);
                break;
            }
        }

        displayShaderCode += generatedShaderCode;
        shaderCode += generatedShaderCode;

        //for (const auto& e : code)
        //{
        //    displayShaderCode += e.second;
        //}
        shaderCode += "}\n";
        displayShaderCode += "}\n";
        std::string delim = "\n";
        std::string::size_type pos = 0, prev = 0;
        while ((pos = displayShaderCode.find(delim, prev)) != std::string::npos)
        {
            codeAsLines.push_back(displayShaderCode.substr(prev, pos - prev));
            prev = pos + delim.size();
        }
        codeAsLines.push_back(displayShaderCode.substr(prev));
    }

    Node* FindNode(ed::NodeId id)
    {
        for (auto& node : m_Nodes)
            if (node.ID == id)
                return &node;

        return nullptr;
    }

    Link* FindLink(ed::LinkId id)
    {
        for (auto& link : m_Links)
            if (link.ID == id)
                return &link;

        return nullptr;
    }

    Pin* FindPin(ed::PinId id)
    {
        if (!id)
            return nullptr;

        for (auto& node : m_Nodes)
        {
            for (auto& pin : node.Inputs)
                if (pin.ID == id)
                    return &pin;

            for (auto& pin : node.Outputs)
                if (pin.ID == id)
                    return &pin;
        }

        return nullptr;
    }

    bool IsPinLinked(ed::PinId id)
    {
        if (!id)
            return false;

        for (auto& link : m_Links)
            if (link.StartPinID == id || link.EndPinID == id)
                return true;

        return false;
    }

    bool CanCreateLink(Pin* a, Pin* b)
    {
        if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
            return false;

        return true;
    }

    //void DrawItemRect(ImColor color, float expand = 0.0f)
    //{
    //    ImGui::GetWindowDrawList()->AddRect(
    //        ImGui::GetItemRectMin() - ImVec2(expand, expand),
    //        ImGui::GetItemRectMax() + ImVec2(expand, expand),
    //        color);
    //};

    //void FillItemRect(ImColor color, float expand = 0.0f, float rounding = 0.0f)
    //{
    //    ImGui::GetWindowDrawList()->AddRectFilled(
    //        ImGui::GetItemRectMin() - ImVec2(expand, expand),
    //        ImGui::GetItemRectMax() + ImVec2(expand, expand),
    //        color, rounding);
    //};

    void BuildNode(Node* node)
    {
        for (auto& input : node->Inputs)
        {
            input.Node = node;
            input.Kind = PinKind::Input;
        }

        for (auto& output : node->Outputs)
        {
            output.Node = node;
            output.Kind = PinKind::Output;
        }
    }

    
#pragma region premadeNodes
    Node* SpawnComment()
    {
        m_Nodes.emplace_back(GetNextId(), "Test Comment");
        m_Nodes.back().Type = NodeType::Comment;
        m_Nodes.back().Size = ImVec2(300, 200);

        return &m_Nodes.back();
    }
#pragma endregion prebuiltNodes

#pragma region shaderNodes

    //#TODO ADDING NEW NODES HERE
    //Float nodes
    Node* spawnFloatConstantNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Constant");
        m_Nodes.back().Type = NodeType::FloatConstant;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();

    }
    Node* spawnFloatAddNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Add");
        m_Nodes.back().Type = NodeType::FloatAdd;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatSubtractNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Subtract");
        m_Nodes.back().Type = NodeType::FloatSubtract;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatMultiplyNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Multiply");
        m_Nodes.back().Type = NodeType::FloatMultiply;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatDivideNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Dividie");
        m_Nodes.back().Type = NodeType::FloatDivide;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatPowNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Power");
        m_Nodes.back().Type = NodeType::FloatPow;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatAbsNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Absolute");
        m_Nodes.back().Type = NodeType::FloatAbsolute;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVectorAbsNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Absolute");
        m_Nodes.back().Type = NodeType::VectorAbsolute;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnSignNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Sign");
        m_Nodes.back().Type = NodeType::Sign;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloorNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Floor");
        m_Nodes.back().Type = NodeType::Floor;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnCeilNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Ceil");
        m_Nodes.back().Type = NodeType::Ceil;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFractNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Fract");
        m_Nodes.back().Type = NodeType::Fract;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnModNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Modulus");
        m_Nodes.back().Type = NodeType::Mod;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatMinNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Min");
        m_Nodes.back().Type = NodeType::FloatMin;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVectorMinNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Min");
        m_Nodes.back().Type = NodeType::VectorMin;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnFloatMaxNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Max");
        m_Nodes.back().Type = NodeType::FloatMax;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVectorMaxNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Max");
        m_Nodes.back().Type = NodeType::VectorMax;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnClampNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Clamp");
        m_Nodes.back().Type = NodeType::Clamp;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "min", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "max", PinType::Float);

        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnMixNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Mix");
        m_Nodes.back().Type = NodeType::Mix;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "alpha", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnSineNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Sine");
        m_Nodes.back().Type = NodeType::Sin;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnCosineNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Cosine");
        m_Nodes.back().Type = NodeType::Cos;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnTanNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Tan");
        m_Nodes.back().Type = NodeType::Tan;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    // Vector nodes
    Node* spawnDotNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Dot Product");
        m_Nodes.back().Type = NodeType::Dot;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "dot", PinType::Float);
        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnCrossNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Cross Product");
        m_Nodes.back().Type = NodeType::Cross;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "cross", PinType::Vector4);
        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnLengthNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Length");
        m_Nodes.back().Type = NodeType::Length;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "length", PinType::Float);
        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnNormaliseNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Normalise");
        m_Nodes.back().Type = NodeType::Normalize;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "normal vector", PinType::Vector4);
        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnCombineNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Combine");
        m_Nodes.back().Type = NodeType::Combine;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "x", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "y", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "z", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "w", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "vec4", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnSplitNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Split");
        m_Nodes.back().Type = NodeType::Split;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "vec4", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "x", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "y", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "z", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "w", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVec4ConstNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Constant");
        m_Nodes.back().Type = NodeType::FloatConstant;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVec4AddNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Add");
        m_Nodes.back().Type = NodeType::VectorAdd;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVec4SubtractNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Subtract");
        m_Nodes.back().Type = NodeType::VectorSubtract;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVec4MultiplyNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Multiply");
        m_Nodes.back().Type = NodeType::VectorMultiply;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVec4DivideNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Divide");
        m_Nodes.back().Type = NodeType::VectorDivide;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "b", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnVec4AbsNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Absolute");
        m_Nodes.back().Type = NodeType::VectorAbs;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "a", PinType::Vector4);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Vector4);

        return &m_Nodes.back();
    }
    //fnSig vec2 position, int frequency, int octaveCount, float persistence, float lacunarity, uint seed
    Node* spawnNoiseNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Perlin Noise");
        m_Nodes.back().Type = NodeType::PerlinNoise;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "position", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "frequency", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "octaveCount", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "persistence", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "lacunarity", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "seed", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "output", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnSimpleNoiseNode()
    {
        m_Nodes.emplace_back(GetNextId(), "Perlin Noise");
        m_Nodes.back().Type = NodeType::SimpleNoise;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "position", PinType::Vector4);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "frequency", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "octaveCount", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "seed", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "output", PinType::Float);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
    Node* spawnOutputNode()
    {
        {
            m_Nodes.emplace_back(GetNextId(), "Output");
            m_Nodes.back().Type = NodeType::Output;
            m_Nodes.back().isShader = true;
            //m_Nodes.back().Inputs.emplace_back(GetNextId(), "Float Output", PinType::Float);
            m_Nodes.back().Inputs.emplace_back(GetNextId(), "Vector Output", PinType::Vector4);
            //m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

            BuildNode(&m_Nodes.back());

            return &m_Nodes.back();
        }
    }
    Node* spawnTimeNode()
    {
        m_Nodes.emplace_back(GetNextId(), "time");
        m_Nodes.back().Type = NodeType::Time;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "time", PinType::Float);

        return &m_Nodes.back();
    }
    Node* spawnUVNode()
    {
        m_Nodes.emplace_back(GetNextId(), "UV");
        m_Nodes.back().Type = NodeType::UV;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "x", PinType::Float);
        m_Nodes.back().Outputs.emplace_back(GetNextId(), "y", PinType::Float);
        m_Nodes.back().lineNumber = 1;

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }

    Node* makeNewNode(NodeType t)
    {
        switch (t)
        {
        case NodeType::FloatConstant:
            return spawnFloatConstantNode();
        case NodeType::FloatAdd:
            return spawnFloatAddNode();
        case NodeType::FloatSubtract:
            return spawnFloatSubtractNode();
        case NodeType::FloatMultiply:
            return spawnFloatMultiplyNode();
        case NodeType::FloatDivide:
            return spawnFloatDivideNode();
        case NodeType::FloatPow:
            return spawnFloatPowNode();
        case NodeType::FloatAbsolute:
            return spawnFloatAbsNode();
        case NodeType::Sign:
            return spawnSignNode();
        case NodeType::Floor:
            return spawnFloorNode();
        case NodeType::Ceil:
            return spawnCeilNode();
        case NodeType::Fract:
            return spawnFractNode();
        case NodeType::Mod:
            return spawnModNode();
        case NodeType::FloatMin:
            return spawnFloatMinNode();
        case NodeType::FloatMax:
            return spawnFloatMaxNode();
        case NodeType::Clamp:
            return spawnClampNode();
        case NodeType::Mix:
            return spawnMixNode();
            //Triganometry
        case NodeType::Sin:
            return spawnSineNode();
        case NodeType::Cos:
            return spawnCosineNode();
        case NodeType::Tan:
            return spawnTanNode();
            //Vector
            // causing errors with drawing nodes, will fix if have time
            //case NodeType::VectorConstant:
            //    code += "vec4 " + nodeVar + " = " + "vec4(" + std::to_string(node.value.x) + ", " + std::to_string(node.value.y) + ", " 
            //        + std::to_string(node.value.z) + ", " + std::to_string(node.value.w) + ");\n";
        case NodeType::VectorAdd:
            return spawnVec4AddNode();;
        case NodeType::VectorSubtract:
            return spawnVec4SubtractNode();
        case NodeType::VectorMultiply:
            return spawnVec4MultiplyNode();
        case NodeType::VectorDivide:
            return spawnVec4DivideNode();
        case NodeType::VectorAbs:
            return spawnVec4AbsNode();
        case NodeType::Dot:
            return spawnDotNode();
        case NodeType::Cross:
            return spawnCrossNode();
        case NodeType::Length:
            return spawnLengthNode();
        case NodeType::Normalize:
            return spawnNormaliseNode();

            //Vector Utilities
        case NodeType::Combine:
            return spawnCombineNode();
            //these nodes don't add any code, so we just stack them up here
        case NodeType::Split:
            return spawnSplitNode();
        case NodeType::UV:
            return spawnUVNode();

            //float perlinNoise(vec2 position, int frequency, int octaveCount, float persistence, float lacunarity, uint seed)
        case NodeType::PerlinNoise:
            return spawnNoiseNode();
        case NodeType::Output:
            return spawnOutputNode();
        case NodeType::SimpleNoise:
            return spawnSimpleNoiseNode();
        case NodeType::Time:
            return spawnTimeNode();
        }
    }
#pragma endregion shaderNodes

    void BuildNodes()
    {
        for (auto& node : m_Nodes)
            BuildNode(&node);
    }

    void OnStart() override
    {

        sdl_window = makeSDLWindow(sdl_width, sdl_height);
        if (Render::init() != 0)
        {
            std::cout << "Failed to init window renderer\n";
            __debugbreak();
        }
        

        ed::Config config;

        config.SettingsFile = "Blueprints.json";

        config.UserPointer = this;

        config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
        {
            auto self = static_cast<Example*>(userPointer);

            auto node = self->FindNode(nodeId);
            if (!node)
                return 0;

            if (data != nullptr)
                memcpy(data, node->State.data(), node->State.size());
            return node->State.size();
        };

        config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
        {
            auto self = static_cast<Example*>(userPointer);

            auto node = self->FindNode(nodeId);
            if (!node)
                return false;

            node->State.assign(data, size);

            self->TouchNode(nodeId);

            return true;
        };

        m_Editor = ed::CreateEditor(&config);
        ed::SetCurrentEditor(m_Editor);

        ed::NavigateToContent();

        BuildNodes();

        m_HeaderBackground = LoadTexture("data/BlueprintBackground.png");
        m_SaveIcon         = LoadTexture("data/ic_save_white_24dp.png");
        m_RestoreIcon      = LoadTexture("data/ic_restore_white_24dp.png");


        //auto& io = ImGui::GetIO();
    }

    void OnStop() override
    {
        auto releaseTexture = [this](ImTextureID& id)
        {
            if (id)
            {
                DestroyTexture(id);
                id = nullptr;
            }
        };

        releaseTexture(m_RestoreIcon);
        releaseTexture(m_SaveIcon);
        releaseTexture(m_HeaderBackground);

        if (m_Editor)
        {
            ed::DestroyEditor(m_Editor);
            m_Editor = nullptr;
        }
    }

    ImColor GetIconColor(PinType type)
    {
        switch (type)
        {
            default:
            case PinType::Flow:     return ImColor(255, 255, 255);
            case PinType::Bool:     return ImColor(220,  48,  48);
            case PinType::Int:      return ImColor( 68, 201, 156);
            case PinType::Float:    return ImColor(147, 226,  74);
            case PinType::String:   return ImColor(124,  21, 153);
            case PinType::Object:   return ImColor( 51, 150, 215);
            case PinType::Function: return ImColor(218,   0, 183);
            case PinType::Delegate: return ImColor(255,  48,  48);
            case PinType::Vector4:  return ImColor(77, 77, 225);
        }
    };

    void DrawPinIcon(const Pin& pin, bool connected, int alpha)
    {
        IconType iconType;
        ImColor  color = GetIconColor(pin.Type);
        color.Value.w = alpha / 255.0f;
        switch (pin.Type)
        {
            case PinType::Flow:     iconType = IconType::Flow;   break;
            case PinType::Bool:     iconType = IconType::Circle; break;
            case PinType::Int:      iconType = IconType::Circle; break;
            case PinType::Float:    iconType = IconType::Circle; break;
            case PinType::String:   iconType = IconType::Circle; break;
            case PinType::Object:   iconType = IconType::Circle; break;
            case PinType::Function: iconType = IconType::Circle; break;
            case PinType::Delegate: iconType = IconType::Square; break;
            case PinType::Vector4:  iconType = IconType::RoundSquare; break;
            default:
                return;
        }

        ax::Widgets::Icon(ImVec2(static_cast<float>(m_PinIconSize), static_cast<float>(m_PinIconSize)), iconType, connected, color, ImColor(32, 32, 32, alpha));
    };

    void ShowStyleEditor(bool* show = nullptr)
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

    void ShowLeftPane(float paneWidth)
    {
        //selectedNodes.clear();
        //selectedLinks.clear();

        auto& io = ImGui::GetIO();

        ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));


        paneWidth = ImGui::GetContentRegionAvail().x;

        static bool showStyleEditor = false;
        ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
        ImGui::Spring(0.0f, 0.0f);
        
        ImGui::Spring(0.0f);
        if (ImGui::Button("Show Flow"))
        {
            for (auto& link : m_Links)
                ed::Flow(link.ID);
        }
        ImGui::Spring();
        if (ImGui::Button("Generate Shader"))
        {
            buildShader();
            std::cout << "compiling shader\n";
            if (program_one.isActive) 
            { 
                if (program_two.updateShader(shaderCode))
                {
                    program_one.isActive = false;
                    program_two.use();
                }
            }
            else
            {
                if (program_one.updateShader(shaderCode))
                {
                    program_two.isActive = false;
                    program_one.use();
                }
            }
        }

        if (ImGui::Button("Zoom to Content"))
            ed::NavigateToContent();
        ImGui::Spring();
        if (ImGui::Button("Edit Style"))
            showStyleEditor = true;
        ImGui::EndHorizontal();
        ImGui::Checkbox("Show Ordinals", &m_ShowOrdinals);

        //#TODO: Save/Load
        if (ImGui::Button("Print state"))
        {
            for (auto& n : m_Nodes)
            {
                std::cout << n.State << "\n";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            loading = false;
            saving = true;
        }
        if (saving)
        {
            char fileName[512] = { '\0' };
            
            if (ImGui::InputText("save", &fileName[0], 512, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string file(fileName);
                std::cout << "Saving to " << file << "\n";
                serialiseNodeTree();
                std::string fp = file + ".shader";
                std::ofstream FILE(fp.c_str());
                FILE << outputJson.dump(4) << std::endl;
                
                saving = false;
            }
            
        }
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            saving = false;
            loading = true;
        }
        if (loading)
        {
            m_Nodes.clear();
            m_Links.clear();
            m_NextId = 1;
            inputJson.clear();
            char fileName[512]{ "\0" };
            if (ImGui::InputText("load", &fileName[0], 512, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::vector<Node> newNodes;
                std::unordered_map<uint64_t, uint64_t> indexMap;
                
                
                std::string file(fileName);

                file += ".shader";
                std::cout << "Loading from " << file << "\n";
                std::ifstream FILE(file.c_str());
                if (!FILE) 
                {
                    std::cout << "Failed to load file";
                    goto exitLoading;
                }
                
                FILE >> inputJson;
                
                for (auto& i : inputJson["nodes"])
                {
                    Node* newNode;
                    uint64_t idx = i.at("index").get<uint64_t>();
                    NodeType type = (NodeType)i.at("type").get<uint64_t>();
                    std::string userName = i.at("userName").get<std::string>();
                    ImVec4 value = {
                        i.at("x").get<float>(),i.at("y").get<float>(),
                        i.at("z").get<float>(),i.at("w").get<float>()
                    };
                    std::string state = i.at("state").get<std::string>();
                    newNode = makeNewNode(type);
                    indexMap[idx] = newNode->ID.Get();                    
                    json jState = json::parse(state);
                    ImVec2 nodePos = ImVec2(jState["location"]["x"], jState["location"]["y"]);
                    ed::SetNodePosition(newNode->ID, nodePos);
                    newNode->value = value;
                    //ed::RestoreNodeState(newNode->ID);
                    newNodes.push_back(*newNode);
                }
                for (auto& i : inputJson["links"])
                {
                    
                    uint64_t startNodeIdx = i.at("startNodeIdx").get<uint64_t>();
                    uint64_t startPinOffset = i.at("startPinOffset").get<uint64_t>();
                    //use inputs as the output pins are generated after the input pins
                    //however, we want the offset into the outputs, so we have to account for this
                    startPinOffset -= newNodes[startNodeIdx].Inputs.size(); 
                    uint64_t endNodeIdx = i.at("endNodeIdx").get<uint64_t>();
                    uint64_t endPinOffset = i.at("endPinOffset").get<uint64_t>();
                    ed::PinId startPin = newNodes[startNodeIdx].Outputs[startPinOffset -1].ID;
                    ed::PinId endPin = newNodes[endNodeIdx].Inputs[endPinOffset -1].ID;
                    m_Links.push_back(Link(GetNextLinkId(),
                        startPin,
                        endPin
                    )
                    );
                    Node* n = FindNode(newNodes[startNodeIdx].ID);
                    m_Links.back().Color = GetIconColor(n->Outputs[startPinOffset - 1].Type);

                }
            exitLoading:

                loading = false;
            }
        }
#ifndef _NDEBUG
        if (ImGui::Button("Item Picker"))
        {
            ImGui::DebugStartItemPicker();
        }
#endif


        if (showStyleEditor)
            ShowStyleEditor(&showStyleEditor);

        std::vector<ed::NodeId> selectedNodes;
        std::vector<ed::LinkId> selectedLinks;
        selectedNodes.resize(ed::GetSelectedObjectCount());
        selectedLinks.resize(ed::GetSelectedObjectCount());

        int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
        int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

        selectedNodes.resize(nodeCount);
        selectedLinks.resize(linkCount);

        int saveIconWidth     = GetTextureWidth(m_SaveIcon);
        int saveIconHeight    = GetTextureWidth(m_SaveIcon);
        int restoreIconWidth  = GetTextureWidth(m_RestoreIcon);
        int restoreIconHeight = GetTextureWidth(m_RestoreIcon);

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetCursorScreenPos(),
            ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
        ImGui::Spacing(); ImGui::SameLine();
        if (0)
        {
            ImGui::TextUnformatted("Nodes");
            ImGui::Indent();
            for (auto& node : m_Nodes)
            {
                ImGui::PushID(node.ID.AsPointer());
                auto start = ImGui::GetCursorScreenPos();

                if (const auto progress = GetTouchProgress(node.ID))
                {
                    ImGui::GetWindowDrawList()->AddLine(
                        start + ImVec2(-8, 0),
                        start + ImVec2(-8, ImGui::GetTextLineHeight()),
                        IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
                }

                bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.ID) != selectedNodes.end();
# if IMGUI_VERSION_NUM >= 18967
                ImGui::SetNextItemAllowOverlap();
# endif
                if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
                {
                    if (io.KeyCtrl)
                    {
                        if (isSelected)
                            ed::SelectNode(node.ID, true);
                        else
                            ed::DeselectNode(node.ID);
                    }
                    else
                        ed::SelectNode(node.ID, false);

                    ed::NavigateToSelection();
                }
                if (ImGui::IsItemHovered() && !node.State.empty())
                    ImGui::SetTooltip("State: %s", node.State.c_str());

                auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer())) + ")";
                auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
                auto iconPanelPos = start + ImVec2(
                    paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
                    (ImGui::GetTextLineHeight() - saveIconHeight) / 2);
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
                if (node.SavedState.empty())
                {
                    if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
                        node.SavedState = node.State;

                    if (ImGui::IsItemActive())
                        drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
                    else if (ImGui::IsItemHovered())
                        drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                    else
                        drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
                }
                else
                {
                    ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
                    drawList->AddImage(m_SaveIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
                }

                ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
# if IMGUI_VERSION_NUM < 18967
                ImGui::SetItemAllowOverlap();
# else
                ImGui::SetNextItemAllowOverlap();
# endif
                if (!node.SavedState.empty())
                {
                    if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
                    {
                        node.State = node.SavedState;
                        ed::RestoreNodeState(node.ID);
                        node.SavedState.clear();
                    }

                    if (ImGui::IsItemActive())
                        drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
                    else if (ImGui::IsItemHovered())
                        drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                    else
                        drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
                }
                else
                {
                    ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
                    drawList->AddImage(m_RestoreIcon, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
                }

                ImGui::SameLine(0, 0);
# if IMGUI_VERSION_NUM < 18967
                ImGui::SetItemAllowOverlap();
# endif
                ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

                ImGui::PopID();
            }
            ImGui::Unindent();


            static int changeCount = 0;

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImGui::GetCursorScreenPos(),
                ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
                ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
            ImGui::Spacing(); ImGui::SameLine();
            ImGui::TextUnformatted("Selection");
            
            ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
            ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
            ImGui::Spring();
            if (ImGui::Button("Deselect All"))
                ed::ClearSelection();
            ImGui::EndHorizontal();
        
        ImGui::Indent();
        for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
        for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
        ImGui::Unindent();

        if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
            for (auto& link : m_Links)
                ed::Flow(link.ID);

        if (ed::HasSelectionChanged())
            ++changeCount;
        }

        // added code here
        ImGui::Text("Generated Shader Code");
        {
            Render::Program* activeShader = nullptr;
            if (!program_one.isActive) activeShader = &program_one;
            else if (!program_two.isActive) activeShader = &program_two;
            if (codeAsLines.size() > 0)
            {
                std::vector<ed::NodeId> selected;
                selected.resize(ed::GetSelectedObjectCount());
                int nodeCount = ed::GetSelectedNodes(selected.data(), static_cast<int>(selected.size()));
                int errLn = -1;
                std::vector<int> selectedLines;

                if (activeShader && !activeShader->errLog.empty())
                {
                    size_t pos = activeShader->errLog.find("0(");
                    size_t ePos = activeShader->errLog.find(") :");
                    std::string errL = "";
                    pos += 2;
                    for (auto& i = pos; i < ePos; i++)
                    {
                        errL += activeShader->errLog[i];
                    }
                    //std::cout << errLn << "\n";
                    size_t process = 0;
                    errLn = std::stoi(errL, &process, 10);
                    errLn -= 102;
                }
                if (selected.size() > 0)
                {
                    for (auto& id : selected)
                    {
                        Node* n = FindNode(id);
                        selectedLines.push_back(n->lineNumber);
                    }
                }
                displayCode(selectedLines, errLn);
            }


        }
        //ImGui::InputTextMultiline("##Shader Output", &displayShaderCode[0], displayShaderCode.size(), ImVec2(500, 500), ImGuiInputTextFlags_ReadOnly);
        //to here
        ImGui::EndChild();
    }

    void OnFrame(float deltaTime) override
    {
        //clear the buffer for the next frame
        glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        UpdateTouch();

        auto& io = ImGui::GetIO();

        ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

        ed::SetCurrentEditor(m_Editor);

        //auto& style = ImGui::GetStyle();

    # if 0
        {
            for (auto x = -io.DisplaySize.y; x < io.DisplaySize.x; x += 10.0f)
            {
                ImGui::GetWindowDrawList()->AddLine(ImVec2(x, 0), ImVec2(x + io.DisplaySize.y, io.DisplaySize.y),
                    IM_COL32(255, 255, 0, 255));
            }
        }
    # endif

        static ed::NodeId contextNodeId      = 0;
        static ed::LinkId contextLinkId      = 0;
        static ed::PinId  contextPinId       = 0;
        static bool createNewNode  = false;
        static Pin* newNodeLinkPin = nullptr;
        static Pin* newLinkPin     = nullptr;

        static float leftPaneWidth  = 450.0f;
        static float rightPaneWidth = 800.0f;
        Splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f);

        ShowLeftPane(leftPaneWidth - 4.0f);

        ImGui::SameLine(0.0f, 12.0f);

        Node* menuOnNode = nullptr;
        //we can extend this here to add new drawing methods for new node types if we need to
        ed::Begin("Node editor");
        {
            auto cursorTopLeft = ImGui::GetCursorScreenPos();

            util::BlueprintNodeBuilder builder(m_HeaderBackground, GetTextureWidth(m_HeaderBackground), GetTextureHeight(m_HeaderBackground));

            for (auto& node : m_Nodes)
            {
                if ((node.Type != NodeType::Blueprint && node.Type != NodeType::Simple) && !node.isShader)
                    continue;

                const auto isSimple = node.Type == NodeType::Simple;

                bool hasOutputDelegates = false;
                for (auto& output : node.Outputs)
                    if (output.Type == PinType::Delegate)
                        hasOutputDelegates = true;

                builder.Begin(node.ID);
                    if (!isSimple)
                    {
                        builder.Header(node.Color);
                            ImGui::Spring(0);
                            if (node.userDefinedName != "")
                            {
                                ImGui::TextUnformatted(node.userDefinedName.c_str());
                            }
                            else
                            {
                                ImGui::TextUnformatted(node.Name.c_str());
                            }
                            
                            ImGui::Spring(1);
                            ImGui::Dummy(ImVec2(0, 28));
                            if (hasOutputDelegates)
                            {
                                ImGui::BeginVertical("delegates", ImVec2(0, 28));
                                ImGui::Spring(1, 0);
                                for (auto& output : node.Outputs)
                                {
                                    if (output.Type != PinType::Delegate)
                                        continue;

                                    auto alpha = ImGui::GetStyle().Alpha;
                                    if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                                        alpha = alpha * (48.0f / 255.0f);

                                    ed::BeginPin(output.ID, ed::PinKind::Output);
                                    ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
                                    ed::PinPivotSize(ImVec2(0, 0));
                                    ImGui::BeginHorizontal(output.ID.AsPointer());
                                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                                    if (!output.Name.empty())
                                    {
                                        ImGui::TextUnformatted(output.Name.c_str());
                                        ImGui::Spring(0);
                                    }
                                    DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                                    ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                                    ImGui::EndHorizontal();
                                    ImGui::PopStyleVar();
                                    ed::EndPin();

                                    //DrawItemRect(ImColor(255, 0, 0));
                                }
                                ImGui::Spring(1, 0);
                                ImGui::EndVertical();
                                ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
                            }
                            else
                                ImGui::Spring(0);
                        builder.EndHeader();
                    }
                    //custom drawing for constant nodes
                    if (node.Type == NodeType::FloatConstant)
                    {
                        ImGui::PushItemWidth(100);
                        ImGui::DragFloat(("##FloatValue" + std::to_string(node.ID.Get())).c_str(), &node.value.x, 0.1f, 0.0f, 1.0f);
                        ImGui::PopItemWidth();
                    }
                    else if (node.Type == NodeType::VectorConstant)
                    {
                        ImGui::PushItemWidth(100);
                        ImGui::DragFloat(("##FloatValue" + std::to_string(node.ID.Get())).c_str(), &node.value.x, 0.1f, 0.0f, 1.0f);
                        
                        ImGui::DragFloat(("##FloatValue" + std::to_string(node.ID.Get())).c_str(), &node.value.y, 0.1f, 0.0f, 1.0f);
                        
                        ImGui::DragFloat(("##FloatValue" + std::to_string(node.ID.Get())).c_str(), &node.value.z, 0.1f, 0.0f, 1.0f);
                        
                        ImGui::DragFloat(("##FloatValue" + std::to_string(node.ID.Get())).c_str(), &node.value.w, 0.1f, 0.0f, 1.0f);
                        ImGui::PopItemWidth();
                        ImVec2 minSize = { 150, 150 };
                        ImVec2 contentSize = ImGui::GetItemRectSize();

                        ImGui::Dummy(ImVec2(std::max(contentSize.x, minSize.x), std::max(contentSize.y, minSize.y)));
                    }
                    for (auto& input : node.Inputs)
                    {
                        auto alpha = ImGui::GetStyle().Alpha;
                        if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
                            alpha = alpha * (48.0f / 255.0f);

                        builder.Input(input.ID);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                        DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
                        ImGui::Spring(0);
                        if (!input.Name.empty())
                        {
                            ImGui::TextUnformatted(input.Name.c_str());
                            ImGui::Spring(0);
                        }
                        if (input.Type == PinType::Bool)
                        {
                             ImGui::Button("Hello");
                             ImGui::Spring(0);
                        }
                        ImGui::PopStyleVar();
                        builder.EndInput();
                    }

                    if (isSimple)
                    {
                        builder.Middle();

                        ImGui::Spring(1, 0);
                        ImGui::TextUnformatted(node.Name.c_str());
                        ImGui::Spring(1, 0);
                    }

                    for (auto& output : node.Outputs)
                    {
                        if (!isSimple && output.Type == PinType::Delegate)
                            continue;

                        auto alpha = ImGui::GetStyle().Alpha;
                        if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
                            alpha = alpha * (48.0f / 255.0f);

                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
                        builder.Output(output.ID);
                        if (output.Type == PinType::String)
                        {
                            static char buffer[128] = "Edit Me\nMultiline!";
                            static bool wasActive = false;

                            ImGui::PushItemWidth(100.0f);
                            ImGui::InputText("##edit", buffer, 127);
                            ImGui::PopItemWidth();
                            if (ImGui::IsItemActive() && !wasActive)
                            {
                                ed::EnableShortcuts(false);
                                wasActive = true;
                            }
                            else if (!ImGui::IsItemActive() && wasActive)
                            {
                                ed::EnableShortcuts(true);
                                wasActive = false;
                            }
                            ImGui::Spring(0);
                        }
                        if (!output.Name.empty())
                        {
                            ImGui::Spring(0);
                            ImGui::TextUnformatted(output.Name.c_str());
                        }
                        ImGui::Spring(0);
                        DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
                        ImGui::PopStyleVar();
                        builder.EndOutput();
                    }
                    
                builder.End();
            }

            //TODO tooltip drawing
            ed::NodeId hoveredNodeId = ed::GetHoveredNode();
            
            if (hoveredNodeId)
            {
                //std::cout << hoveredNodeId.Get() << std::endl;
                Node* node = nullptr;
                for (auto& n : m_Nodes)
                {
                    if (n.ID == hoveredNodeId) node = &n;
                }
                if (node != nullptr) 
                { 
                    drawTooltip(*node); 
                }

            }
            //end

            //removed houdini and tree nodes drawing from here

            for (auto& node : m_Nodes)
            {
                if (node.Type != NodeType::Comment)
                    continue;

                const float commentAlpha = 0.75f;

                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
                ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
                ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
                ed::BeginNode(node.ID);
                ImGui::PushID(node.ID.AsPointer());
                ImGui::BeginVertical("content");
                ImGui::BeginHorizontal("horizontal");
                ImGui::Spring(1);
                ImGui::TextUnformatted(node.Name.c_str());
                ImGui::Spring(1);
                ImGui::EndHorizontal();
                ed::Group(node.Size);
                ImGui::EndVertical();
                ImGui::PopID();
                ed::EndNode();
                ed::PopStyleColor(2);
                ImGui::PopStyleVar();

                if (ed::BeginGroupHint(node.ID))
                {
                    //auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

//ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);

auto min = ed::GetGroupMin();
//auto max = ed::GetGroupMax();

ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
ImGui::BeginGroup();
ImGui::TextUnformatted(node.Name.c_str());
ImGui::EndGroup();

auto drawList = ed::GetHintBackgroundDrawList();

auto hintBounds = ImGui_GetItemRect();
auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

drawList->AddRectFilled(
    hintFrameBounds.GetTL(),
    hintFrameBounds.GetBR(),
    IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

drawList->AddRect(
    hintFrameBounds.GetTL(),
    hintFrameBounds.GetBR(),
    IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);

//ImGui::PopStyleVar();
                }
                ed::EndGroupHint();
            }

            for (auto& link : m_Links)
                ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

            if (!createNewNode)
            {
                if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
                {
                    auto showLabel = [](const char* label, ImColor color)
                    {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
                        auto size = ImGui::CalcTextSize(label);

                        auto padding = ImGui::GetStyle().FramePadding;
                        auto spacing = ImGui::GetStyle().ItemSpacing;

                        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

                        auto rectMin = ImGui::GetCursorScreenPos() - padding;
                        auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

                        auto drawList = ImGui::GetWindowDrawList();
                        drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
                        ImGui::TextUnformatted(label);
                    };

                    ed::PinId startPinId = 0, endPinId = 0;
                    if (ed::QueryNewLink(&startPinId, &endPinId))
                    {
                        auto startPin = FindPin(startPinId);
                        auto endPin = FindPin(endPinId);

                        newLinkPin = startPin ? startPin : endPin;

                        if (startPin->Kind == PinKind::Input)
                        {
                            std::swap(startPin, endPin);
                            std::swap(startPinId, endPinId);
                        }

                        if (startPin && endPin)
                        {
                            if (endPin == startPin)
                            {
                                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            }
                            else if (endPin->Kind == startPin->Kind)
                            {
                                showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
                            }
                            else if (endPin->Node == startPin->Node)
                            {
                                showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
                                ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
                            }
                            //else if (endPin->Type != startPin->Type)
                            //{
                            //    showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
                            //    ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
                            //}
                            else
                            {
                                showLabel("+ Create Link", ImColor(32, 45, 32, 180));
                                //custom link checking code to disallow multiple end pins)
                                
                                if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
                                {
                                    for (auto it = m_Links.begin(); it != m_Links.end(); it++)
                                    {
                                        Link l = *it;
                                        if (l.EndPinID == endPin->ID)
                                        {
                                            m_Links.erase(it);
                                            break;
                                        }
                                    }
                                    m_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
                                    m_Links.back().Color = GetIconColor(startPin->Type);
                                }
                            }
                        }
                    }

                    ed::PinId pinId = 0;
                    if (ed::QueryNewNode(&pinId))
                    {
                        newLinkPin = FindPin(pinId);
                        if (newLinkPin)
                            showLabel("+ Create Node", ImColor(32, 45, 32, 180));

                        if (ed::AcceptNewItem())
                        {
                            createNewNode  = true;
                            newNodeLinkPin = FindPin(pinId);
                            newLinkPin = nullptr;
                            ed::Suspend();
                            ImGui::OpenPopup("Create New Node");
                            ed::Resume();
                        }
                    }
                }
                else
                    newLinkPin = nullptr;

                ed::EndCreate();

                if (ed::BeginDelete())
                {
                    ed::NodeId nodeId = 0;
                    while (ed::QueryDeletedNode(&nodeId))
                    {
                        if (ed::AcceptDeletedItem())
                        {
                            auto id = std::find_if(m_Nodes.begin(), m_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
                            if (id != m_Nodes.end())
                                m_Nodes.erase(id);
                        }
                    }

                    ed::LinkId linkId = 0;
                    while (ed::QueryDeletedLink(&linkId))
                    {
                        if (ed::AcceptDeletedItem())
                        {
                            auto id = std::find_if(m_Links.begin(), m_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
                            if (id != m_Links.end())
                                m_Links.erase(id);
                        }
                    }
                }
                ed::EndDelete();
            }

            ImGui::SetCursorScreenPos(cursorTopLeft);
        }

    # if 1
        auto openPopupPosition = ImGui::GetMousePos();
        ed::Suspend();
        if (ed::ShowNodeContextMenu(&contextNodeId))
            ImGui::OpenPopup("Node Context Menu");
        else if (ed::ShowPinContextMenu(&contextPinId))
            ImGui::OpenPopup("Pin Context Menu");
        else if (ed::ShowLinkContextMenu(&contextLinkId))
            ImGui::OpenPopup("Link Context Menu");
        else if (ed::ShowBackgroundContextMenu())
        {
            ImGui::OpenPopup("Create New Node");
            newNodeLinkPin = nullptr;
        }
        ed::Resume();

        ed::Suspend();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        if (ImGui::BeginPopup("Node Context Menu"))
        {
            auto node = FindNode(contextNodeId);

            ImGui::TextUnformatted("Node Context Menu");
            ImGui::Separator();
            if (node)
            {
                char buffer[128];
                strncpy_s(buffer, node->userDefinedName.c_str(), sizeof(buffer));
                if (ImGui::InputText("##NodeName", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    node->userDefinedName = buffer;
                }
                ImGui::Text("ID: %p", node->ID.AsPointer());
                ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
                ImGui::Text("Inputs: %d", (int)node->Inputs.size());
                ImGui::Text("Outputs: %d", (int)node->Outputs.size());
            }
            else
                ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteNode(contextNodeId);
            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Pin Context Menu"))
        {
            auto pin = FindPin(contextPinId);

            ImGui::TextUnformatted("Pin Context Menu");
            ImGui::Separator();
            if (pin)
            {
                ImGui::Text("ID: %p", pin->ID.AsPointer());
                if (pin->Node)
                    ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
                else
                    ImGui::Text("Node: %s", "<none>");
            }
            else
                ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

            ImGui::EndPopup();
        }

        if (ImGui::BeginPopup("Link Context Menu"))
        {
            auto link = FindLink(contextLinkId);

            ImGui::TextUnformatted("Link Context Menu");
            ImGui::Separator();
            if (link)
            {
                ImGui::Text("ID: %p", link->ID.AsPointer());
                ImGui::Text("From: %p", link->StartPinID.AsPointer());
                ImGui::Text("To: %p", link->EndPinID.AsPointer());
            }
            else
                ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
            ImGui::Separator();
            if (ImGui::MenuItem("Delete"))
                ed::DeleteLink(contextLinkId);
            ImGui::EndPopup();
        }
        //TODO extend nodes here
        if (ImGui::BeginPopup("Create New Node"))
        {
            auto newNodePostion = openPopupPosition;
            //ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

            //auto drawList = ImGui::GetWindowDrawList();
            //drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

            Node* node = nullptr;
            //node = makeNewNode(); //this currently isn't working so that's fun
            
            if (ImGui::MenuItem("Output"))
                node = spawnOutputNode();
            if (ImGui::MenuItem("Input UV"))
                node = spawnUVNode();
            if (ImGui::BeginMenu("Float Maths"))
            {
                if (ImGui::MenuItem("Constant"))
                    node = spawnFloatConstantNode();
                if (ImGui::MenuItem("Add"))
                    node = spawnFloatAddNode();
                if (ImGui::MenuItem("Subtract"))
                    node = spawnFloatSubtractNode();
                if (ImGui::MenuItem("Multiply"))
                    node = spawnFloatMultiplyNode();
                if (ImGui::MenuItem("Divide"))
                    node = spawnFloatDivideNode();
                if (ImGui::MenuItem("Power"))
                    node = spawnFloatPowNode();
                if (ImGui::MenuItem("Sign"))
                    node = spawnSignNode();
                if (ImGui::MenuItem("Floor"))
                    node = spawnFloorNode();
                if (ImGui::MenuItem("Ceil"))
                    node = spawnCeilNode();
                if (ImGui::MenuItem("Fract"))
                    node = spawnFractNode();
                if (ImGui::MenuItem("Modulo"))
                    node = spawnModNode();
                if (ImGui::MenuItem("Min"))
                    node = spawnFloatMinNode();
                if (ImGui::MenuItem("Max"))
                    node = spawnFloatMaxNode();
                if (ImGui::MenuItem("Clamp"))
                    node = spawnClampNode();
                ImGui::EndPopup();
            }

            if (ImGui::BeginMenu("Trig"))
            {
                if (ImGui::MenuItem("Sine"))
                    node = spawnSineNode();
                if (ImGui::MenuItem("Cosine"))
                    node = spawnCosineNode();
                if (ImGui::MenuItem("Tangent"))
                    node = spawnTanNode();
                ImGui::EndPopup();
            }

            if (ImGui::BeginMenu("Vector Maths"))
            {
                if (ImGui::MenuItem("Add"))
                    node = spawnVec4AddNode();
                if (ImGui::MenuItem("Subtract"))
                    node = spawnVec4SubtractNode();
                if (ImGui::MenuItem("Multiply"))
                    node = spawnVec4MultiplyNode();
                if (ImGui::MenuItem("Divide"))
                    node = spawnVec4DivideNode();
                if (ImGui::MenuItem("Absolute"))
                    node = spawnVec4AbsNode();
                if (ImGui::MenuItem("Dot Product"))
                    node = spawnDotNode();
                if (ImGui::MenuItem("Cross Product"))
                    node = spawnCrossNode();
                if (ImGui::MenuItem("Length"))
                    node = spawnLengthNode();
                if (ImGui::MenuItem("Normalise"))
                    node = spawnNormaliseNode();
                if (ImGui::MenuItem("Combine"))
                    node = spawnCombineNode();
                if (ImGui::MenuItem("Split"))
                    node = spawnSplitNode();
                ImGui::EndPopup();
            }
            
            //if (ImGui::MenuItem("Combine"))
            //    node = spawnCombineNode();
            
            //this node is causing rendering errors so we're ignoring it for now
            //if (ImGui::MenuItem("Vector Constant"))
            //    node = spawnVec4ConstNode();
            ImGui::Separator();
            if (ImGui::MenuItem("Comment"))
                node = SpawnComment();
            if (ImGui::MenuItem("Simple Noise"))
                node = spawnSimpleNoiseNode();
            if (ImGui::MenuItem("Perlin Noise"))
                node = spawnNoiseNode();
            if (ImGui::MenuItem("Time"))
                node = spawnTimeNode();
            

            if (node)
            {
                BuildNodes();

                createNewNode = false;

                ed::SetNodePosition(node->ID, newNodePostion);

                if (auto startPin = newNodeLinkPin)
                {
                    auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

                    for (auto& pin : pins)
                    {
                        if (CanCreateLink(startPin, &pin))
                        {
                            auto endPin = &pin;
                            if (startPin->Kind == PinKind::Input)
                                std::swap(startPin, endPin);

                            m_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
                            m_Links.back().Color = GetIconColor(startPin->Type);

                            break;
                        }
                    }
                }
            }

            ImGui::EndPopup();
        }
        else
            createNewNode = false;
        ImGui::PopStyleVar();
        ed::Resume();
    # endif


    /*
        cubic_bezier_t c;
        c.p0 = pointf(100, 600);
        c.p1 = pointf(300, 1200);
        c.p2 = pointf(500, 100);
        c.p3 = pointf(900, 600);

        auto drawList = ImGui::GetWindowDrawList();
        auto offset_radius = 15.0f;
        auto acceptPoint = [drawList, offset_radius](const bezier_subdivide_result_t& r)
        {
            drawList->AddCircle(to_imvec(r.point), 4.0f, IM_COL32(255, 0, 255, 255));

            auto nt = r.tangent.normalized();
            nt = pointf(-nt.y, nt.x);

            drawList->AddLine(to_imvec(r.point), to_imvec(r.point + nt * offset_radius), IM_COL32(255, 0, 0, 255), 1.0f);
        };

        drawList->AddBezierCurve(to_imvec(c.p0), to_imvec(c.p1), to_imvec(c.p2), to_imvec(c.p3), IM_COL32(255, 255, 255, 255), 1.0f);
        cubic_bezier_subdivide(acceptPoint, c);
    */

        ed::End();

        auto editorMin = ImGui::GetItemRectMin();
        auto editorMax = ImGui::GetItemRectMax();

        

        
        
        float time = (float)ImGui::GetTime();
        if (m_ShowOrdinals)
        {
            int nodeCount = ed::GetNodeCount();
            std::vector<ed::NodeId> orderedNodeIds;
            orderedNodeIds.resize(static_cast<size_t>(nodeCount));
            ed::GetOrderedNodeIds(orderedNodeIds.data(), nodeCount);


            auto drawList = ImGui::GetWindowDrawList();
            drawList->PushClipRect(editorMin, editorMax);

            int ordinal = 0;
            for (auto& nodeId : orderedNodeIds)
            {
                auto p0 = ed::GetNodePosition(nodeId);
                auto p1 = p0 + ed::GetNodeSize(nodeId);
                p0 = ed::CanvasToScreen(p0);
                p1 = ed::CanvasToScreen(p1);


                ImGuiTextBuffer builder;
                builder.appendf("#%d", ordinal++);

                auto textSize   = ImGui::CalcTextSize(builder.c_str());
                auto padding    = ImVec2(2.0f, 2.0f);
                auto widgetSize = textSize + padding * 2;

                auto widgetPosition = ImVec2(p1.x, p0.y) + ImVec2(0.0f, -widgetSize.y);

                drawList->AddRectFilled(widgetPosition, widgetPosition + widgetSize, IM_COL32(100, 80, 80, 190), 3.0f, ImDrawFlags_RoundCornersAll);
                drawList->AddRect(widgetPosition, widgetPosition + widgetSize, IM_COL32(200, 160, 160, 190), 3.0f, ImDrawFlags_RoundCornersAll);
                drawList->AddText(widgetPosition + padding, IM_COL32(255, 255, 255, 255), builder.c_str());
            }

            drawList->PopClipRect();
        }

        //std::cout << time << "\n";
        //we do all the rendering of the shader at the end of the program
        if (program_one.isActive)
        {
            program_one.use();
            program_one.setFloat("i_time", time);
            Render::testRender(&program_one);
        }
        else if (program_two.isActive)
        {
            program_two.use();
            program_two.setFloat("i_time", time);
            Render::testRender(&program_two);
        }
        else
        {
            Render::testRender();
        }
        

        SDL_GL_SwapWindow(sdl_window);
        //ImGui::ShowTestWindow();
        //ImGui::ShowMetricsWindow();
    }

    int                  m_NextId = 1;
    const int            m_PinIconSize = 24;
    std::vector<Node>    m_Nodes;
    std::vector<Link>    m_Links;
    ImTextureID          m_HeaderBackground = nullptr;
    ImTextureID          m_SaveIcon = nullptr;
    ImTextureID          m_RestoreIcon = nullptr;
    const float          m_TouchTime = 1.0f;
    std::map<ed::NodeId, float, NodeIdLess> m_NodeTouchTime;
    bool                 m_ShowOrdinals = false;

    //my stuff here for rendering and extending the codebase to be a better shader editor
    SDL_Window*          sdl_window;
    int                  sdl_width = 640;
    int                  sdl_height = 480;
    Render::Program      program_one;
    Render::Program      program_two;
    std::vector<std::string> codeAsLines;
    int currentLineNumber = 5;

    std::vector<Node> copiedNodes;
    std::vector<Link> copiedLinks;
    json outputJson;
    json inputJson;

    bool saving = false;
    bool loading = false;


};

int Main(int argc, char** argv)
{
    Example exampe("Blueprints", argc, argv);

    if (exampe.Create())
        return exampe.Run();

    return 0;
}