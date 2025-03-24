#pragma once
#define SDL_MAIN_HANDLED

#include <SDL.h>
#include "Program.h"
#include "Nodes.h"


SDL_Window* makeSDLWindow(int width = 640, int height = 480)
{
    std::cout << "Loading SDL\n";
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        __debugbreak(); //failed to initialise SDL D-:
    }

    auto window = SDL_CreateWindow("Output Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    //SDL_SetWindowAlwaysOnTop(window, SDL_TRUE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    if (SDL_GL_CreateContext(window) == NULL)
    {
        auto err = SDL_GetError();
        std::cout << "SDL Error : " << err;
        __debugbreak();
    }
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, SDL_TRUE);
    SDL_GL_SetSwapInterval(0);
    return window;
}

std::string getTooltip(NodeType t)
{
    switch (t)
    {
    case NodeType::FloatConstant:
        return "A floating point constant value";
    case NodeType::FloatAdd:
        return "Addition node for two floating point numbers";
        break;
    case NodeType::FloatSubtract:
        return "Subtraction node for two floating point numbers";
        break;
    case NodeType::FloatMultiply:
        return "Multiplication node for two floating point numbers";
        break;
    case NodeType::FloatDivide:
        return "Division node for two floating point numbers";
        break;
    case NodeType::FloatPow:
        return "Raises a to the power of b (a^b)";
        break;
    case NodeType::FloatAbsolute:
        return "Get the absolute value of a (|a|)";
        break;
    case NodeType::Sign:
        return "Returns the sign of the input value\nif x > 0.0, returns +1; if x == 0.0, returns 0; if x < 0.0, returns -1;";
        break;
    case NodeType::Floor:
        return "Finds the nearest integer less than or equal to the input value";
        break;
    case NodeType::Ceil:
        return "Finds the nearest integer greater than or equal to the input value";
        break;
    case NodeType::Fract:
        return "Computes the fractional part of the input value";
        break;
    case NodeType::Mod:
        return "Calculates a modulo b (a % b)";
        break;
    case NodeType::FloatMin:
        return "Returns the lesser of a and b";
        break;
    case NodeType::FloatMax:
        return "Returns the greater of a and b";
        break;
    case NodeType::Clamp:
        return "Contraints a within min and max (min <= a <= max)";
        break;
    case NodeType::Mix:
        return "Linearly interpolate between a and b using alpha as a factor\nat Alpha = 0, output is a, at Alpha = 1, output is b";
    case NodeType::Sin:
        return "Returns the value sin(a)";
        break;
    case NodeType::Cos:
        return "Returns the value cos(a)";
        break;
    case NodeType::Tan:
        return "Returns the value tan(a)";
        break;
    case NodeType::VectorAdd:
        return "Adds two vector4's together (a + b)";
        break;
    case NodeType::VectorSubtract:
        return "Subtracts b from a (a - b)";
        break;
    case NodeType::VectorMultiply:
        return "Multiplies a by b (a * b)";
        break;
    case NodeType::VectorDivide:
        return "Divides a by b (a / b)";
        break;
    case NodeType::VectorAbs:
        return "Returns the absolute value of the vector a (|a|)";
        break;
    case NodeType::Dot:
        return "Returns the dot product of the two vectors a and b (a . b)";
        break;
    case NodeType::Cross:
        return "Returns the cross product of the two vectors a and b (a x b)";
        break;
    case NodeType::Length:
        return "Returns the length of the vector a (sqrt(a.x^2 + a.y^2 + a.z^2))";
        break;
    case NodeType::Normalize:
        return "Normalises the vector a to the range [-1 -> 1]";
        break;

        //Vector Utilities
    case NodeType::Combine:
        return "Creates a vec4 from 4 input floats";
        break;
        
    case NodeType::Split:
        return "Splits a vec4 into it's component floats";
    case NodeType::UV:
        return "Gets the object UV coordinates for the shader";
    case NodeType::Time:
        return "Gets the current time of the shader, used for animations";
        break;

    case NodeType::PerlinNoise:
        return "Creates a pseudorandom pattern for use in shaders";
        break;
    case NodeType::Output:
        return "Outputs the final colour of the shader to the graphics card";
        break;
    case NodeType::SimpleNoise:
        return "Creates a pseudorandom pattern for use in shaders";
        break;
    }
}