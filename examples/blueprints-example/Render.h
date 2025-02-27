#pragma once
#include <GL/gl3w.h> 
#include <SDL.h>
#include <iostream>
#include <string>
#include <vector>

//
// Render Module expets objects as GameObject
// A GameObject has at most, 32 textures (OpenGL limitation of 32 samplers)
// 
// 
//

#define test

namespace Render
{
	int init();

	//This is the render module, which will hold the render state
	class Render
	{
		Render(SDL_Window*);
		SDL_Window* window;
	};

	
#ifdef test
#pragma region TEST

	int testPopulate();
	
	void testRender();
	
	
#endif
	
}


