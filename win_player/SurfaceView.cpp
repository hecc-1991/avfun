#include "SurfaceView.h"


#include "glad/glad.h"

#include "codec/ffmpeg_config.h"
#include "codec/AVFVideoReader.h"
#include "codec/AVFAudioReader.h"

#include "LogUtil.h"
#include "Utils.h"
#include "render/GLProgram.h"
#include "render/GLTexture.h"
#include "codec/AVVideoFrame.h"

#define WINDOW_WIGTH 1280
#define WINDOW_HEIGTH 720

using namespace avf;

struct SurfaceView::Impl
{
    float progress{ 0 };
    SP<AVPlayer> player{nullptr};

    UP<render::GLProgram> program;
    unsigned int VAO;

    SP<avf::codec::AVVideoFrame> vframe;

    bool isInit{false};

    void init();

    void draw();
};

void checkGLError(int line = -1) {
    auto err = glGetError();
    if (err != 0)
        LOG_ERROR("line: %d, err: %d", line, err);
}

void SurfaceView::Impl::init(){

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


    auto size = player->GetSize();
    //AVFSizei size = {1202,720};

    auto vertices = Utils::FitIn(WINDOW_WIGTH, WINDOW_HEIGTH, size.width, size.height);

    program = make_up<render::GLProgram>(_vertex_shader, _fragment_shader);

    int indices[] = {
            0, 1, 2,
            0, 2, 3,
    };

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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //checkGLError(__LINE__);
}

void SurfaceView::Impl::draw() {
    if (player->GetStat() < PLAYER_STAT::INIT)
        return;

    if(!isInit){
        init();
        isInit = true;
    }

    //glClearColor(0.2f, 0.3f, 0.3f, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if(vframe == nullptr){
        auto size = player->GetSize();
        vframe = make_sp<avf::codec::AVVideoFrame>(size.width, size.height);
    }

    vframe->reset();

    player->GetFrame(vframe);

    auto texture = make_up<avf::render::GLTexture>(vframe->GetWidth(), vframe->GetHeight(),
                                                   vframe->get());

    program->Use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    //checkGLError(__LINE__);

}

void SurfaceView::SetPlayer(SP<AVPlayer> player) {
    _impl->player = player;
}


void SurfaceView::Draw() {
    _impl->draw();
}

SurfaceView::SurfaceView(): _impl(make_up<SurfaceView::Impl>()) {

}

SurfaceView::~SurfaceView() {

}