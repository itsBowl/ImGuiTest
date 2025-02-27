#pragma once
#include "Node.h"


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

Node* spawnTestNode()
{
    m_Nodes.emplace_back(GetNextId(), "StringTest");
    m_Nodes.back().Type = NodeType::Simple;
    m_Nodes.back().isShader = true;
    m_Nodes.back().Inputs.emplace_back(GetNextId(), "StringInput", PinType::String);
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

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
    m_Nodes.back().Outputs.emplace_back(GetNextId(), "vec3", PinType::Vector);

    BuildNode(&m_Nodes.back());

    return &m_Nodes.back();
}

Node* spawnOutputNode()
{
    {
        m_Nodes.emplace_back(GetNextId(), "Output");
        m_Nodes.back().Type = NodeType::Output;
        m_Nodes.back().isShader = true;
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Float Output", PinType::Float);
        m_Nodes.back().Inputs.emplace_back(GetNextId(), "Vector Output", PinType::Vector);
        //m_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::String);

        BuildNode(&m_Nodes.back());

        return &m_Nodes.back();
    }
}