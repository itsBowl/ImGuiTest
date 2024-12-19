#include "Window.h"

Window::Window()
{
	std::cout << "Starting window";
	int width = 800;
	int height = 640;
	assert(SDL_Init(SDL_INIT_VIDEO) == 0 && "failed to init SDL");
	window = SDL_CreateWindow("Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	glContext = SDL_GL_CreateContext(window);
	if (glContext == NULL)
	{
		std::cout << "failed to set GL_CONTEXT";
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, SDL_TRUE);
	SDL_GL_SetSwapInterval(0);

	if (gl3wInit() != GL3W_OK)
	{
		std::cout << "failed to init openGL";
	}

	glDebugMessageCallback(message_callback, nullptr);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glCullFace(GL_BACK);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard
		| ImGuiConfigFlags_NavEnableGamepad;

	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init();
}

void Window::makeImGuiWindow(Framebuffer& framebuffer, std::string name = "NULL")
{
	ImGui::Begin(name.c_str());
	ImGui::BeginChild("a");
	float w = ImGui::GetContentRegionAvail().x;
	float h = ImGui::GetContentRegionAvail().y;

	ImGui::Image(
		(ImTextureID)framebuffer.getFrameTexture(),
		ImGui::GetContentRegionAvail(),
		ImVec2(0, 1),
		ImVec2(1, 0)
	);
}
