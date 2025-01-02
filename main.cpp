#include "main.h"
#include "Window.h"
#include "Framebuffer.h"
#include "NodeEditor.h"

namespace ed = ax::NodeEditor;


int main(int argc, char** argv)
{
	Window window;
	Framebuffer framebuffer(window.size());
	Framebuffer editor(window.size());
	NodeEditor nodeEditor;


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

		nodeEditor.DoEditor(context);
				

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		
		//swap :-)
		SDL_GL_SwapWindow(window.getWindow());


	} while (!close);

	ed::DestroyEditor(context);
	return 0;
}