#pragma once
#define SDL_MAIN_HANDLED

#include <GL/gl3w.h> 
#include <SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/transform.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>



namespace Render
{

	
	class Program
	{
	public:

		bool isActive = false;
		std::string errLog = "";
		bool updateShader(std::string);

#pragma warning( suppress : 26495)
		Program() = default;
		Program(std::string directory) { makeProgram(directory); }

		void use() { glUseProgram(id); isActive = true; }

		void setFloat4(const std::string name, glm::vec4 value) const
		{
			glUniform4fv(glGetUniformLocation(id, name.c_str()), 1,
				glm::value_ptr(value));
		}

		void setMat4(const std::string name, glm::mat4 value) const
		{
			glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1,
				GL_FALSE, glm::value_ptr(value));
		}

		void setInt(const std::string name, int value) const
		{
			GLuint uniformLocation = glGetUniformLocation(id, name.c_str());
			glUniform1i(uniformLocation, (GLint)value);
		}

		void setFloat(const std::string name, float value) const
		{
			GLuint location = glGetUniformLocation(id, name.c_str());
			glUniform1f(location, value);
		}

		GLuint getID() { return id; }
	private:
		GLuint id = 0;
		bool makeProgramFromString(const std::string&);
		void makeProgram(std::string);
		GLuint compileShader(GLenum, const std::vector<char>&);
		bool compileStatus(GLuint);

		GLuint vertexID = 0;
		GLuint fragmentID = 0;
		

		//const char* vertexShaderSource =
		//	"#version 450 core\n"
		//	"layout (location = 0) in vec3 aPos;\n"
		//	"layout (location = 1) in vec2 uvMap;\n"
		//	"out vec2 vUV;\n"
		//	"void main()\n"
		//	"{\n"
		//	"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		//	"	vUV = uvMap;\n"
		//	"}\0";

		const std::string vertexShaderSource = R"SHADER(
#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 uvMap;
uniform float i_time;
out vec2 vUV;
out float time;
void main()
{
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
	time = i_time;
	vUV = uvMap;
})SHADER";

	};

}

