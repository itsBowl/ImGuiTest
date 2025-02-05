#include "main.h"
#include "Window.h"
#include "Framebuffer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "NodeEditor.h"

namespace ed = ax::NodeEditor;


void startEditor(NodeEditor*);

int main(int argc, char** argv)
{
	Window window;
	Framebuffer framebuffer(window.size());
	Framebuffer editor(window.size());
	NodeEditor nodeEditor;
	IMGUI_CHECKVERSION();
	auto imGuiContext = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         
	ImGui::SetCurrentContext(imGuiContext);
	ImGui_ImplSDL2_InitForOpenGL(window.getWindow(), window.getContext());
	ImGui_ImplOpenGL3_Init();

	//old test code
	ed::Config config;
	config.SettingsFile = "Test.json";
	auto context = ed::CreateEditor(&config);
	
	
	bool close = false, resized = false;
	bool showDemo = true;
	
	do
	{
		SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
		SDL_PumpEvents();

		SDL_Event event;
		while (SDL_PollEvent(&event) != 0)
		{
			switch (event.type)
			{
			case SDL_QUIT:
				close = true;
			case SDL_WINDOWEVENT_RESIZED:
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				resized = true;
			}
			ImGui_ImplSDL2_ProcessEvent(&event);
			
			

		}
		//render stage

		//clear window backbuffer
		glClearColor(0.05f, 0.1f, 0.f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//draw to output stage
		
		framebuffer.Clear();
		editor.Clear();
		
		//imgui structure stage

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		window.makeImGuiWindow(framebuffer, "Render");
		ImGui::EndChild();
		ImGui::End();

		auto& io = ImGui::GetIO();

		ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

		ImGui::Separator();

		//nodeEditor.DoEditor();
		nodeEditor.onFrame(io.DeltaTime);
				

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		
		//swap :-)
		SDL_GL_SwapWindow(window.getWindow());


	} while (!close);

	ed::DestroyEditor(context);
	return 0;
}

void startEditor(NodeEditor* e)
{
	ed::Config cfg;

	cfg.SettingsFile = "Settings.json";

	cfg.UserPointer = e;

	cfg.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
	{
		auto self = static_cast<NodeEditor*>(userPointer);

		auto node = self->findNode(nodeId);
		if (!node)
			return 0;

		if (data != nullptr)
			memcpy(data, node->state.data(), node->state.size());
		return node->state.size();
	};

	cfg.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
	{
		auto self = static_cast<NodeEditor*>(userPointer);

		auto node = self->findNode(nodeId);
		if (!node)
			return false;

		node->state.assign(data, size);

		self->touchNode(nodeId);

		return true;
	};

	e->ctx = ed::CreateEditor(&cfg);
	ed::SetCurrentEditor(e->ctx);
	e->setup();


}