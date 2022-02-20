#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <SDL.h>
#include <glad/glad.h>
#include <LogUtil.h>
#include <render/GLProgram.h>
#include <render/GLTexture.h>
#include <codec/AVFVideoReader.h>
#include <codec/AVVideoFrame.h>

using namespace cv;
using namespace avf;
using namespace avf::codec;
using namespace avf::render;

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

void checkGLError(int line = -1)
{
    auto err = glGetError();
    if (err != 0)
        LOG_ERROR("line: %d, err: %d",line, err);
}

void initGLContext(SDL_Window** mainwindow, SDL_GLContext *maincontext) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		sdldie("Unable to initialize SDL");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
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

std::array<float,20> fitIn(int sw,int sh,int tw,int th){
    float l,r,t,b;
    auto ra_s = float(sw)/float(sh);
    auto ra_t = float(tw)/float(th);
    if(ra_s >= ra_t){
        l = (1.0 - ra_t / ra_s)/2.0;
        r = l + ra_t / ra_s;
        t = 0;
        b = 1.0;
    }else{
        l = 0;
        r = 1.0;
        t = (1.0 - ra_s/ra_t)/2.0;
        b = t + ra_s/ra_t;
    }

    l = 2 * l -1.0;
    r = 2 * r -1.0;
    t = 2 * t -1.0;
    b = 2 * b -1.0;

    std::array<float,20> vertices = {
            // positions            // texture coords
            r, t, 0.0f,     1.0f, 1.0f,   // top right
            r, b, 0.0f,     1.0f, 0.0f,  // bottom right
            l, b, 0.0f,     0.0f, 0.0f,   // bottom left
            l, t, 0.0f,     0.0f, 1.0f,    // top left
    };

    return vertices;

};


void renderTexture() {
	constexpr auto _vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

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

	auto image = imread("/Users/hecc/Documents/hecc/dev/avfun/resources/image01.jpg", IMREAD_UNCHANGED);
	if (image.channels() == 3)
		cvtColor(image, image, COLOR_BGR2BGRA);
	auto texture = make_up<GLTexture>(image.cols,image.rows, image.data);

    auto img_w = image.cols;
    auto img_h = image.rows;

    checkGLError(__LINE__);

	auto program = make_up<GLProgram>(_vertex_shader, _fragment_shader);
    checkGLError(__LINE__);

    /*
    float vertices[] = {
        // positions          // colors           // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // top right
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // top left
    };
    */


    auto vertices = fitIn(WINDOW_WIGTH,WINDOW_HEIGTH,img_w,img_h);

    unsigned int indices[] = {
		0,1,2,
		0,2,3,
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

    unsigned int EBO;
    glGenBuffers(1, &EBO);


    glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//	glEnableVertexAttribArray(1);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	program->Use();

	glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());


    glBindVertexArray(VAO);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

}

void renderVideo(SDL_Window* mainwindow) {

	constexpr auto _vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
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

	constexpr auto video_filename = "/Users/hecc/Documents/hecc/dev/avfun/resources/zhu.MP4";

	auto vr = AVFVideoReader::Make(video_filename);

	vr->SetupDecoder();

    auto size = vr->GetSize();

    auto vertices = fitIn(WINDOW_WIGTH,WINDOW_HEIGTH,size.width,size.height);

    auto program = make_up<GLProgram>(_vertex_shader, _fragment_shader);

//	float vertices[] = {
//		// positions          // colors           // texture coords
//		 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,   // top right
//		 1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
//		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,   // bottom left
//		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f    // top left
//	};

	int indices[] = {
		0,1,2,
		0,2,3,
	};

	unsigned int VAO;
	glGenVertexArrays(1, &VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);

    unsigned int EBO;
    glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
//	glEnableVertexAttribArray(1);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//glViewport(0, 0, frameW, frameH);

    SDL_Event event;
    bool quit = false;

	while (!quit)
	{

        glClearColor(0.2f, 0.3f, 0.3f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

		auto vframe = vr->ReadNextFrame();
		if (vframe == nullptr)break;

		auto texture = make_up<GLTexture>( vframe->GetWidth(), vframe->GetHeight(), vframe->get());

		program->Use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		SDL_GL_SwapWindow(mainwindow);

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
		SDL_Delay(1);
	}

}

int main(int argc,char* args[]) {

	SDL_Window* mainwindow = nullptr;
	SDL_GLContext maincontext;

	initGLContext(&mainwindow,&maincontext);

    SDL_Event event;

    while(0){
        SDL_PollEvent(&event);

        switch(event.type) {
            case SDL_QUIT:
                break;

            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        break;
                }
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        //renderTriangle();
        renderTexture();
        SDL_GL_SwapWindow(mainwindow);
    }



#if 1
	renderVideo(mainwindow);
#endif

	SDL_GL_DeleteContext(maincontext);
	SDL_DestroyWindow(mainwindow);
	SDL_Quit();

	return 0;
}