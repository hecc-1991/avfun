#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <SDL.h>
#include <glad/glad.h>
#include <LogUtil.h>
#include <render/GLProgram.h>
#include <render/GLTexture.h>
#include <codec/AVVideoReader.h>
#include <codec/AVVideoFrame.h>

using namespace cv;
using namespace avfun;
using namespace avfun::codec;
using namespace avfun::render;

#define WINDOW_WIGTH 1280
#define WINDOW_HEIGTH 720

void sdldie(const char* msg)
{
	LOG_ERROR("%s: %s", msg, SDL_GetError());
	SDL_Quit();
	exit(1);
}

void checkSDLError(int line = -1)
{
#ifndef NDEBUG
	const char* error = SDL_GetError();
	if (*error != '\0')
	{
		printf("SDL Error: %s\n", error);
		if (line != -1)
			printf(" + line: %i\n", line);
		SDL_ClearError();
	}
#endif
}

void initGLContext(SDL_Window** mainwindow, SDL_GLContext *maincontext) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		sdldie("Unable to initialize SDL");
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	*mainwindow = SDL_CreateWindow("sdl-glad", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		WINDOW_WIGTH, WINDOW_HEIGTH, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	if (!(*mainwindow))
		sdldie("Unable to create window");

	checkSDLError(__LINE__);

	*maincontext = SDL_GL_CreateContext(*mainwindow);

	//SDL_GL_MakeCurrent(*mainwindow,maincontext);

	checkSDLError(__LINE__);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		sdldie("Failed to initialize the OpenGL context.");
	}

	LOG_INFO("OpenGL version loaded: %d.%d", GLVersion.major, GLVersion.minor);

	SDL_GL_SetSwapInterval(1);

}

void renderTriangle() {

	constexpr auto _vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
)";

	constexpr auto _fragment_shader = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 uColor;

void main()
{
    FragColor = vec4(uColor, 1.0);
}
)";

	auto program = make_up<GLProgram>(_vertex_shader, _fragment_shader);

	float vertices[] = {
	-0.5f, -0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	 0.0f,  0.5f, 0.0f
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	program->Use();
	float color[] = {1.0,1.0,0.0};
	program->SetVec("uColor", color,3);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);

}

void renderTexture() {
	constexpr auto _vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

	constexpr auto _fragment_shader = R"(
#version 330 core
out vec4 FragColor;
  
in vec3 ourColor;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

	auto image = imread("C:/hecc/develop/github/avfun/avfun/example/resource/image01.jpg", IMREAD_UNCHANGED);
	if (image.channels() == 3)
		cvtColor(image, image, COLOR_BGR2RGBA);
	auto texture = make_up<GLTexture>(image.cols,image.rows, image.data);

	auto program = make_up<GLProgram>(_vertex_shader, _fragment_shader);
	/*
	float vertices[] = {
		// positions          // colors           // texture coords
		 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
		 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left 
	};
	*/
	float vertices[] = {
		// positions          // colors           // texture coords
		 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // top right
		 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   // bottom left
		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f    // top left 
	};

	int indcies[] = {
		0,1,2,
		0,2,3,
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	program->Use();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());

	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indcies);

}

void renderVideo(SDL_Window* mainwindow) {

	constexpr auto _vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;
out vec2 TexCoord;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

	constexpr auto _fragment_shader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord);
}
)";

	constexpr auto video_filename = "C:/hecc/develop/github/avfun/avfun/example/resource/video01.mp4";

	auto vr = AVVideoReader::Make(video_filename);

	vr->SetupDecoder();

	auto program = make_up<GLProgram>(_vertex_shader, _fragment_shader);

	float vertices[] = {
		// positions          // colors           // texture coords
		 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // top right
		 1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   // bottom left
		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f    // top left 
	};

	int indcies[] = {
		0,1,2,
		0,2,3,
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);

	//glViewport(0, 0, frameW, frameH);

	while (true)
	{
		auto vframe = vr->ReadNextFrame();
		if (vframe == nullptr)break;

		auto texture = make_up<GLTexture>( vframe->GetWidth(), vframe->GetHeight(), vframe->get());

		program->Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indcies);

		SDL_GL_SwapWindow(mainwindow);
		SDL_Delay(15);
	}

}

int main(int argc,char* args[]) {

	SDL_Window* mainwindow = nullptr;
	SDL_GLContext maincontext;

	initGLContext(&mainwindow,&maincontext);


	glClearColor(0.2f, 0.3f, 0.3f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	// ##
	//renderTriangle();
	//renderTexture();
	//SDL_GL_SwapWindow(mainwindow);
	//SDL_Delay(5000);
	// ##

	renderVideo(mainwindow);

	SDL_GL_DeleteContext(maincontext);
	SDL_DestroyWindow(mainwindow);
	SDL_Quit();

	return 0;
}