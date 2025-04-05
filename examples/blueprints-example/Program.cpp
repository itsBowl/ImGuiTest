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
		auto start{ std::chrono::steady_clock::now() };
		if (!makeProgramFromString(newSrc)) 
		{
			auto finish{ std::chrono::steady_clock::now() };
			std::chrono::duration<double> dur {finish - start};
			std::cout << "compile time on fail: " << dur << "\n";
			std::cout << "failed to make new program\n";
			return false; 
		}

		auto finish{ std::chrono::steady_clock::now() };
		std::chrono::duration<double> dur {finish - start};
		std::cout << "compile time on success: " << dur << "\n";
		

		return true;
	}

	bool Program::makeTarget(std::string& src)
	{
		id = glCreateProgram();
		if (id == 0)
		{
			std::cout << "Failed to create new ID";
			return false;
		}
		std::vector<char> vertString(vertexShaderSource.begin(), vertexShaderSource.end());
		vertString.push_back('\0');
		vertexID = compileShader(GL_VERTEX_SHADER, vertString);

		std::vector<char> fragString(src.begin(), src.end());
		fragString.push_back('\0');
		fragmentID = compileShader(GL_FRAGMENT_SHADER, fragString);

		if (fragmentID == 0 || vertexID == 0)
		{
			std::cout << "Failed to create shaders\n";
			return false;
		}

		glAttachShader(id, vertexID);
		glAttachShader(id, fragmentID);
		glLinkProgram(id);
		glDeleteShader(vertexID);
		glDeleteShader(fragmentID);



		if (!compileStatus(id))
		{
			std::cout << "Failed to link program\n";
			return false;
		}
		return true;
	}

	bool Program::makeProgramFromString(const std::string& src)
	{
		if (vertexID) { glDetachShader(id, vertexID); vertexID = 0; }
		if (fragmentID) { glDetachShader(id, fragmentID); fragmentID = 0; }
		if (id)
			glDeleteShader(id);
		id = 0;
		id = glCreateProgram();
		if (id == 0)
		{
			std::cout << "Failed to create new ID";
			return false;
		}
		std::vector<char> vertString(vertexShaderSource.begin(), vertexShaderSource.end());
		vertString.push_back('\0');
		vertexID = compileShader(GL_VERTEX_SHADER, vertString);
		
		std::vector<char> fragString(src.begin(), src.end());
		fragString.push_back('\0');
		fragmentID = compileShader(GL_FRAGMENT_SHADER, fragString);

		if (fragmentID == 0 || vertexID == 0) 
		{ 
			std::cout << "Failed to create shaders\n";
			return false; 
		}

		glAttachShader(id, vertexID);
		glAttachShader(id, fragmentID);
		glLinkProgram(id);
		glDeleteShader(vertexID);
		glDeleteShader(fragmentID);


		
		if (!compileStatus(id)) 
		{ 
			std::cout << "Failed to link program\n";
			return false;
		}
		return true;
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
		if (glIsShader(shader)) 
		{ 
			glGetShaderiv(shader, GL_SHADER_TYPE, &type); 
		}
		else if (glIsProgram(shader)) 
		{ 
			type = GL_PROGRAM; 
		}


		if (type == GL_PROGRAM) 
		{ 
			glGetProgramiv(shader, GL_LINK_STATUS, &result); 
		}
		else 
		{ 
			glGetShaderiv(shader, GL_COMPILE_STATUS, &result); 
		}

		if (result == GL_FALSE)
		{
			if (type != GL_PROGRAM)
			{

				glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &srcLength);
				if (srcLength > 0) {
					std::vector<char> shaderSrc(srcLength);
					glGetShaderSource(shader, srcLength, nullptr, shaderSrc.data());
					std::cout << "Shader Source:\n" << shaderSrc.data() << std::endl;
				}

				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
				if (logLength > 0) {
					std::vector<char> shaderError(logLength);
					glGetShaderInfoLog(shader, logLength, nullptr, shaderError.data());
					std::cout << std::format("Error compiling shader: {}\n", shaderError.data());
					errLog = std::string(shaderError.data()) + "\n";
				}
			}
			else
			{
				glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength);
				if (logLength > 0) 
				{
					std::vector<char> programError(logLength);
					glGetProgramInfoLog(shader, logLength, nullptr, programError.data());
					std::cout << std::format("Error linking program: {}\n", programError.data());
				}
				
			}

			GLenum err;
			while ((err = glGetError()) != GL_NO_ERROR) {
				std::cerr << "OpenGL Error: " << err << std::endl;
			}
			return false;
		}
		return true;
	}


}
	
