#include "Program.h"
#include "FileManagement.h"

#define VALID_SHADER_FILE_MIN_COUNT 2
#define VALID_SHADER_FILE_MAX_COUNT 2

namespace Render
{
	void Program::makeProgram(std::string dir)
	{
		namespace FM = FileManagment;
		std::vector<std::pair<std::string, std::string>> filePaths;
		if (!FM::getFilesInFolder(&filePaths, dir)) 
		{ std::cout << "didn't read anything\n"; return; }

		if (filePaths.size() < VALID_SHADER_FILE_MIN_COUNT ||
			filePaths.size() > VALID_SHADER_FILE_MAX_COUNT)
		{ std::cout << "incorrect number of files in shader dir\n"; return; }

		std::string fragment, vertex;
		for (auto& entry : filePaths)
		{
			if (entry.second == ".frag") fragment = entry.first;
			if (entry.second == ".vert") vertex = entry.first;
		}

		id = glCreateProgram();
		std::vector<char> vertexCode, fragmentCode;
		if (!FM::readFile(vertex, &vertexCode)) std::cout << "failed to read vertex file\n"; return;
		if (!FM::readFile(fragment, &fragmentCode)) std::cout << "Failed to read fragment file\n"; return;

		auto vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
		auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

		glAttachShader(id, vertexShader);
		glAttachShader(id, fragmentShader);
		glLinkProgram(id);
		if (!compileStatus(id))
		{
			std::cout << "failed to link shaders";
			return;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);
	}

	bool Program::updateShader(std::string newSrc)
	{
		if (!0)

		return true;
	}

	bool Program::makeProgramFromString(const std::string& src)
	{
		// TODO: Write this function
		return false;
	}

	GLuint Program::compileShader(GLenum type, const std::vector<char>& src)
	{
		GLuint shader = glCreateShader(type);
		const char* srcPtr = src.data();
		glShaderSource(shader, 1, &srcPtr, nullptr);
		glCompileShader(shader);
		if (!compileStatus(shader)) return 0;
		return shader;
	}

	bool Program::compileStatus(GLuint shader)
	{
		GLint result = GL_FALSE;
		int type = 0, logLength = 0, srcLength = 0;
		if (glIsShader(shader)) { glGetShaderiv(shader, GL_SHADER_TYPE, &type); }
		else if (glIsProgram(shader)) { type = GL_PROGRAM; }

		if (type == GL_PROGRAM) { glGetProgramiv(shader, GL_LINK_STATUS, &result); }
		else { glGetShaderiv(shader, GL_COMPILE_STATUS, &result); }

		if (result == GL_FALSE)
		{
			if (type == GL_PROGRAM)
			{
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
				std::string shaderError((logLength > 1) ? logLength : 1, '\0');
				glGetShaderInfoLog(shader, logLength, NULL, shaderError.data());

				glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &srcLength);
				std::string shaderSrc((srcLength > 1) ? srcLength : 1, '\0');
				glGetShaderSource(shader, srcLength, NULL, shaderSrc.data());

				std::cout << shaderSrc << std::endl;
				std::cout << (std::format("Error compiling shader: {}", shaderError));
			}
			else
			{
				glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
				std::string programError((logLength > 1) ? logLength : 1, '\0');
				glGetProgramInfoLog(shader, logLength, NULL, programError.data());
				std::cout << (std::format("Error linking program: {}", programError));
			}
			return false;
		}
		return true;
	}


}
	
