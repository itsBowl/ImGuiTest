#include "Render.h"

namespace Render
{
	//Message callback from https://github.com/fendevel/Guide-to-Modern-OpenGL-Functions#detailed-messages-with-debug-output
	static void message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, GLchar const* message, void const* user_param)
	{
		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
		auto const src_str = [source]() {
			switch (source)
			{
			case GL_DEBUG_SOURCE_API: return "API";
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "WINDOW SYSTEM";
			case GL_DEBUG_SOURCE_SHADER_COMPILER: return "SHADER COMPILER";
			case GL_DEBUG_SOURCE_THIRD_PARTY: return "THIRD PARTY";
			case GL_DEBUG_SOURCE_APPLICATION: return "APPLICATION";
			case GL_DEBUG_SOURCE_OTHER: return "OTHER";
			default: return "";
			}
		}();

		auto const type_str = [type]() {
			switch (type)
			{
			case GL_DEBUG_TYPE_ERROR: return "ERROR";
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "DEPRECATED_BEHAVIOR";
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "UNDEFINED_BEHAVIOR";
			case GL_DEBUG_TYPE_PORTABILITY: return "PORTABILITY";
			case GL_DEBUG_TYPE_PERFORMANCE: return "PERFORMANCE";
			case GL_DEBUG_TYPE_MARKER: return "MARKER";
			case GL_DEBUG_TYPE_OTHER: return "OTHER";
			default: return "";
			}
		}();

		auto const severity_str = [severity]() {
			switch (severity) {
			case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
			case GL_DEBUG_SEVERITY_LOW: return "LOW";
			case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
			case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
			default: return "";
			}
		}();
		std::cout << src_str << ", " << type_str << ", " << severity_str << ", 0x" << std::hex << id << ": " << message << '\n';
	}

	int init()
	{
		if (gl3wInit() != GL3W_OK) return 1;
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(message_callback, nullptr);
		//glEnable(GL_CULL_FACE);
		//glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_LEQUAL);
		//glCullFace(GL_BACK);
#ifdef RENDER_TEST_DATA
		testPopulate();
#endif
		//glFrontFace(GL_CCW);
		return 0;
	}

	Render::Render(SDL_Window* window) : window(window) {}

	std::vector<GLuint> shaderID;



#ifdef RENDER_TEST_DATA
	int success;
	char log[512];

	float vertices[] = {
	 0.5f,  0.5f, 0.0f,  // top right
	 0.5f, -0.5f, 0.0f,  // bottom right
	-0.5f, -0.5f, 0.0f,  // bottom left
	-0.5f,  0.5f, 0.0f   // top left 
	};

	float triVerts[] =
	{
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		3.0f, -1.0f, 0.0f,  2.0f, 0.0f,
		-1.0f, 3.0f, 0.0f, 0.0f, 2.0f
	};

	unsigned int indices[] = {  // note that we start from 0!
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
	};

	GLuint vao;
	GLuint vbo;
	GLuint ibo;
	GLuint vertexShader;
	GLuint fragShader;
	GLuint program;
	int testPopulate()
	{
		const char* vertexShaderSource =
			"#version 450 core\n"
			"layout (location = 0) in vec3 aPos;\n"
			"layout (location = 1) in vec2 uvMap;\n"
			"out vec2 vUV;\n"
			"void main()\n"
			"{\n"
			"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
			"	vUV = uvMap;\n"
			"}\0";

		const char* fragShaderSource =
			"#version 450 core\n"
			"in vec2 vUV;\n"
			"out vec4 FragColor;\n"

			"void main()\n"
			"{\n"
			"FragColor = vec4(vUV.x, vUV.y, 0.0f, 1.0f);\n"
			"}\0";
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);

		//glGenBuffers(1, &ibo);
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3* sizeof(float)));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);


		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, log);
			std::cout << "ERROR SHADER COMPILE FAILED: " << log << std::endl;
			return 1;
		}

		fragShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragShader, 1, &fragShaderSource, NULL);
		glCompileShader(fragShader);
		glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragShader, 512, NULL, log);
			std::cout << "ERROR SHADER COMPILE FAILED: " << log << std::endl;
			return 2;
		}

		program = glCreateProgram();
		glAttachShader(program, vertexShader);
		glAttachShader(program, fragShader);
		glLinkProgram(program);

		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if (!success)
		{
			glGetProgramInfoLog(program, 512, NULL, log);
			std::cout << "ERROR PROGRAM LINK FAILED: " << log << std::endl;
			return 3;
		}

		glDeleteShader(vertexShader);
		glDeleteShader(fragShader);

		return 0;
	}

	void testRender(Program* p)
	{
		if (p)
		{
			glUseProgram(p->getID());
		}
		else { glUseProgram(program); }
		
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
#endif
}


