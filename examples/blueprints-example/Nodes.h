#pragma once

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
    FloatPow,
    //triganomatry
    Sin,
    Cos,
    Tan,
    //Vector
    Combine,
    Split,

    //Input
    UV,
    //Output
    Output
};


