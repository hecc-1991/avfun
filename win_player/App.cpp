#include "App.h"
#include "LogUtil.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#include <glad/glad.h>

#include "AVPlayer.h"

#include "ControlView.h"
#include "SurfaceView.h"

using namespace avf;

#define Window_Width 1280
#define Window_Height 720

class AppInner : public App
{
public:

	virtual void Run() override;

private:

};

/*
	auto ii = imread("C:\\hecc\\develop\\github\\avfun\\win_player\\logo32.png",-1);
	for (size_t i = 0; i < 32; i++)
	{
		for (size_t j = 0; j < 32; j++)
		{
			auto& e = ii.at<Vec4b>(i, j);
			int b = int(15.0 / 255.0 * e[0]);
			int g = int(15.0 / 255.0 * e[1]) << 4;
			int r = int(15.0 / 255.0 * e[2]) << 8;
			int a = int(15.0/255.0 * e[3]) << 12;
			//printf("0x%x,", rgba);
			printf("0x%x,", a+b+g+r);
		}
		printf("\n");
	}
 */
Uint16 ICON_PIXEL[32 * 32] = {  // ...or with raw pixel data:
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x333,0x5111,0x9111,0xc111,0xd111,0xf111,0xf111,0xd111,0xc111,0x9111,0x5111,0x333,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2222,0x9111,0xe111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xe111,0x9111,0x2111,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x111,0x8111,0xe111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xe111,0x8111,0x111,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x2111,0xc111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xc111,0x2222,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x2111,0xd111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xd111,0x2111,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x2111,0xd111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xd111,0x2222,0x0,0x0,0x0,
0x0,0x0,0x111,0xc111,0xf111,0xf111,0xf111,0xf333,0xf555,0xf222,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf222,0xf555,0xf333,0xf111,0xf111,0xf111,0xc111,0x111,0x0,0x0,
0x0,0x0,0x8111,0xf111,0xf111,0xf111,0xf111,0xfaaa,0xfeee,0xfeee,0xf999,0xf222,0xf111,0xf222,0xf333,0xf444,0xf444,0xf333,0xf222,0xf111,0xf222,0xf999,0xfeee,0xfeee,0xfaaa,0xf111,0xf111,0xf111,0xf111,0x8111,0x0,0x0,
0x0,0x2222,0xe111,0xf111,0xf111,0xf111,0xf111,0xfccc,0xfeee,0xfeee,0xfeee,0xfddd,0xfccc,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfccc,0xfddd,0xfeee,0xfeee,0xfeee,0xfccc,0xf111,0xf111,0xf111,0xf111,0xe111,0x2111,0x0,
0x0,0x9111,0xf111,0xf111,0xf111,0xf111,0xf111,0xfccc,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfccc,0xf111,0xf111,0xf111,0xf111,0xf111,0x9111,0x0,
0x333,0xe111,0xf111,0xf111,0xf111,0xf111,0xf111,0xfaaa,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfaaa,0xf111,0xf111,0xf111,0xf111,0xf111,0xe111,0x333,
0x5111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf322,0xfddd,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfddd,0xf323,0xf111,0xf111,0xf111,0xf111,0xf111,0x5111,
0x9111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf999,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf999,0xf111,0xf111,0xf111,0xf111,0xf111,0x9111,
0xb111,0xf111,0xf111,0xf111,0xf111,0xf111,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xf111,0xf111,0xc111,
0xd111,0xf111,0xf111,0xf111,0xf111,0xf333,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf333,0xf111,0xf111,0xf111,0xf111,0xd111,
0xf111,0xf111,0xf111,0xf111,0xf111,0xf333,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf434,0xf111,0xf111,0xf111,0xf111,0xf111,
0xf111,0xf111,0xf111,0xf111,0xf111,0xf333,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf333,0xf111,0xf111,0xf111,0xf111,0xf111,
0xd111,0xf111,0xf111,0xf111,0xf111,0xf111,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xf111,0xf111,0xd111,
0xc111,0xf111,0xf111,0xf111,0xf111,0xf111,0xfaaa,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfaaa,0xf111,0xf111,0xf111,0xf111,0xf111,0xc111,
0x9111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf555,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf555,0xf111,0xf111,0xf111,0xf111,0xf111,0x9111,
0x5111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf999,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf999,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0x5111,
0x333,0xe111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf888,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf888,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xe111,0x222,
0x0,0x9111,0xf111,0xf111,0xf777,0xfaaa,0xf333,0xf111,0xf111,0xf333,0xf888,0xfbbb,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfbbb,0xf888,0xf333,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0x9111,0x0,
0x0,0x2222,0xe111,0xf111,0xf222,0xfaaa,0xfddd,0xf333,0xf111,0xf111,0xf111,0xf111,0xf555,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf555,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xe111,0x2222,0x0,
0x0,0x0,0x8111,0xf111,0xf111,0xf111,0xfccc,0xfccc,0xf222,0xf111,0xf111,0xf111,0xfbbb,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfbbb,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0x8111,0x0,0x0,
0x0,0x0,0x111,0xc111,0xf111,0xf111,0xf666,0xfeee,0xfccc,0xf777,0xf666,0xf999,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xc111,0x333,0x0,0x0,
0x0,0x0,0x0,0x1111,0xd111,0xf111,0xf111,0xfbbb,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xd111,0x2111,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x2111,0xd111,0xf111,0xf111,0xf777,0xfbbb,0xfccc,0xfbbb,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xf111,0xf111,0xf111,0xd111,0x2111,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x1111,0xc111,0xf111,0xf111,0xf111,0xf111,0xf111,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xf111,0xf111,0xc111,0x2222,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x111,0x8111,0xe111,0xf111,0xf111,0xf111,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xf111,0xf111,0xe111,0x8111,0x111,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x2222,0x9111,0xe111,0xf111,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xf111,0xe111,0x9111,0x2222,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x333,0x8bbb,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0xfeee,0x7bbb,0x333,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,

};

void AppInner::Run() {

	LOG_INFO("##################### App Run #####################");

	// Setup SDL
// (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
// depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		LOG_ERROR("Error: %s", SDL_GetError());
		return;
	}

	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 330";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	SDL_Window* window = SDL_CreateWindow("AVFunPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, Window_Width, Window_Height, window_flags);
	SDL_Surface* surface;     // Declare an SDL_Surface to be filled in with pixel data from an image file
	//surface = SDL_CreateRGBSurfaceFrom((void*)pixels, 32, 32, 32, 32*4, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
	surface = SDL_CreateRGBSurfaceFrom(ICON_PIXEL, 32, 32, 16, 32 * 2, 0x0f00, 0x00f0, 0x000f, 0xf000);
	SDL_SetWindowIcon(window, surface);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Initialize OpenGL loader
	bool err = gladLoadGL() == 0;
	if (err)
	{
		LOG_ERROR("Failed to initialize OpenGL loader!");
		return;
	}

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Our state
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    auto player = make_sp<AVPlayer>();

	auto controlView = make_up<ControlView>();
	controlView->SetPlayer(player);

	auto surfaceView = make_up<SurfaceView>();
    surfaceView->SetPlayer(player);


    // Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
				done = true;
		}

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();

//		ImGui::ShowDemoWindow();

//		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		controlView->Draw();

		// Rendering
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);

        surfaceView->Draw();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

}

UP<App> App::Make() {
	return make_up<AppInner>();
}