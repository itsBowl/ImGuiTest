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
    Mix,

    //triganomatry
    Sin,
    Cos,
    Tan,
    //Vector
    VectorConstant,
    VectorAdd,
    VectorSubtract,
    VectorMultiply,
    VectorDivide,
    Dot,
    Cross,
    Length,
    Normalize,
    Scale,

    Combine,
    Split,

    PerlinNoise,

    //Input
    UV,
    //Output
    Output
};


