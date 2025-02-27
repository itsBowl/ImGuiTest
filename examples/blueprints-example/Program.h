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



namespace Render
{
	class Program
	{
	public:

		bool isActive = false;
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
	private:
		GLuint id = 0;
		bool makeProgramFromString(const std::string&);
		void makeProgram(std::string);
		GLuint compileShader(GLenum, const std::vector<char>&);
		bool compileStatus(GLuint);
		

	};

}

