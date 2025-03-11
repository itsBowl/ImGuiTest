#pragma once

enum class PinType
{
    Flow,
    Bool,
    Int,
    Float,
    Vector,
    Vector4,
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
    //commonFunc
    FloatAbsolute,
    VectorAbsolute,
    Sign,
    Floor,
    Ceil,
    Fract,
    Mod,
    FloatMin,
    VectorMin,
    FloatMax,
    VectorMax,
    Clamp,
    Mix, //this one could be a little painful so may be excluded

    //triganomatry
    Sin,
    Cos,
    Tan,
    //Vector
    VectorConstant,
    Dot,
    Cross,
    Length,
    Normalize,

    Combine,
    Split,

    //Input
    UV,
    //Output
    Output
};


