#include "AVPlayer.h"

#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <array>

#include <SDL.h>

#include "glad/glad.h"

#include "codec/ffmpeg_config.h"
#include "codec/AVFVideoReader.h"
#include "codec/AVFAudioReader.h"

#include "LogUtil.h"
#include "render/GLProgram.h"
#include "render/GLTexture.h"
#include "codec/AVVideoFrame.h"

using namespace std::chrono_literals;
using namespace avf::codec;

namespace avf {

    typedef struct Clock {
        double pts {0};           /* clock base */
        double pts_drift{0};     /* clock base minus time at which we updated the clock */
        double last_updated{0};
        int serial{-1};           /* clock is based on a packet with this serial */
        int paused{0};
    } Clock;



    struct VideoState {

        uint8_t *audio_buf{NULL};
        int audio_buf_size{0};
        int audio_buf_index{0};
        int nb_samples{0};
        double audio_pts{0.0};

        double frame_timer{0.0};
        Clock vidclk;
        Clock audclk;

        UP<VideoReader> video_reader;
        UP<AudioReader> audio_reader;

    };

    struct AVPlayer::Impl {

        VideoState* videoState;

        std::thread _th_demux;
        std::thread _th_devid;
        std::thread _th_deaud;
        std::thread _th_play_aud;




        //////////////////////////////////////////////////////////////////////////

        void init_clock(Clock *c)
        {
            double time = av_gettime_relative() / 1000000.0;
            c->pts = 0;
            c->last_updated = time;
            c->pts_drift = c->pts - time;
            c->serial = -1;
        }

        void open_video(std::string_view filename) {

            videoState = new VideoState();

            init_clock(&videoState->audclk);
            init_clock(&videoState->vidclk);

            videoState->video_reader = VideoReader::Make(filename);
            videoState->video_reader->SetupDecoder();

            videoState->audio_reader = AudioReader::Make(filename);
            videoState->audio_reader->SetupDecoder();

            //解复用线程
//            std::thread th(&AVPlayer::Impl::demux, this, filename);
//            _th_demux = std::move(th);
        }

        void close_video(){
            videoState = new VideoState();

        }

        static void fill_audio(void *udata, Uint8 *stream, int len) {
            VideoState *vs = (VideoState *) udata;


            double audio_callback_time = av_gettime_relative() / 1000000.0;

            memset(stream, 0, len);

            while (len > 0) {
                if (vs->audio_buf_index >= vs->audio_buf_size) {
                    auto af = vs->audio_reader->ReadNextFrame();

                    vs->audio_buf = af->buf;
                    vs->audio_buf_size = af->buf_size;
                    vs->nb_samples = af->nb_samples;

                    vs->audio_buf_index = 0;
                }

                int len1 = len > vs->audio_buf_size ? vs->audio_buf_size : len;
                memcpy(stream, vs->audio_buf, len1);

                vs->audio_buf += len1;
                vs->audio_buf_index += len1;

                stream += len1;
                len -= len1;

                vs->audio_pts += 1.0 * vs->nb_samples / SUPPORT_AUDIO_SAMPLE_RATE * len1 / vs->audio_buf_size;

            }

            auto set_clock_at = [](Clock *c, double pts, double time)
            {
                c->pts = pts;
                c->last_updated = time;
                c->pts_drift = c->pts - time;
            };

            set_clock_at(&vs->audclk, vs->audio_pts,  audio_callback_time);

        }

        int open_device() {
            SDL_AudioSpec wanted;

            /* Set the audio format */
            wanted.freq = SUPPORT_AUDIO_SAMPLE_RATE;
            wanted.format = AUDIO_S16;
            wanted.channels = SUPPORT_AUDIO_CHANNELS;    /* 1 = mono, 2 = stereo */
            wanted.samples = SUPPORT_AUDIO_SAMPLE_NUM;  /* Good low-latency value for callback */
            wanted.callback = AVPlayer::Impl::fill_audio;
            wanted.userdata = videoState;

            /* Open the audio device, forcing the desired format */
            if (SDL_OpenAudio(&wanted, NULL) < 0) {
                fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
                return (-1);
            }
            return (0);
        }

        void play_audio() {
            open_device();
            SDL_PauseAudio(0);

            while (true) {
                SDL_Delay(100);
            }

            SDL_CloseAudio();
        }

        void playAudio() {
            //解复用线程
            std::thread th(&AVPlayer::Impl::play_audio, this);

            _th_play_aud = std::move(th);
        }


        std::array<float, 20> fitIn(int sw, int sh, int tw, int th) {
            float l, r, t, b;
            auto ra_s = float(sw) / float(sh);
            auto ra_t = float(tw) / float(th);
            if (ra_s >= ra_t) {
                l = (1.0 - ra_t / ra_s) / 2.0;
                r = l + ra_t / ra_s;
                t = 0;
                b = 1.0;
            } else {
                l = 0;
                r = 1.0;
                t = (1.0 - ra_s / ra_t) / 2.0;
                b = t + ra_s / ra_t;
            }

            l = 2 * l - 1.0;
            r = 2 * r - 1.0;
            t = 2 * t - 1.0;
            b = 2 * b - 1.0;

            std::array<float, 20> vertices = {
                    // positions            // texture coords
                    r, t, 0.0f, 1.0f, 1.0f,   // top right
                    r, b, 0.0f, 1.0f, 0.0f,  // bottom right
                    l, b, 0.0f, 0.0f, 0.0f,   // bottom left
                    l, t, 0.0f, 0.0f, 1.0f,    // top left
            };

            return vertices;

        };

#define WINDOW_WIGTH 1280
#define WINDOW_HEIGTH 720

        void initGLContext(SDL_Window **mainwindow, SDL_GLContext *maincontext) {
            if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                LOG_ERROR("Unable to initialize SDL");
                return;
            }

            SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
            SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

            *mainwindow = SDL_CreateWindow("sdl-glad", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                           WINDOW_WIGTH, WINDOW_HEIGTH, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

            if (!(*mainwindow))
                LOG_ERROR("Unable to create window");


            *maincontext = SDL_GL_CreateContext(*mainwindow);

            //SDL_GL_MakeCurrent(*mainwindow,maincontext);

            if (!gladLoadGLLoader((GLADloadproc) SDL_GL_GetProcAddress)) {
                LOG_ERROR("Failed to initialize the OpenGL context.");
            }

            LOG_INFO("OpenGL version loaded: %d.%d", GLVersion.major, GLVersion.minor);

            SDL_GL_SetSwapInterval(1);

        }

        void checkGLError(int line = -1) {
            auto err = glGetError();
            if (err != 0)
                LOG_ERROR("line: %d, err: %d", line, err);
        }

        PicFrame* refresh_video(){

            auto get_clock = [](Clock *c)->double{
                if (c->paused) {
                    return c->pts;
                } else {
                    double time = av_gettime_relative() / 1000000.0;
                    return c->pts_drift + time;
                }
            };

            auto set_clock = [](Clock *c, double pts)
            {
                double time = av_gettime_relative() / 1000000.0;

                c->pts = pts;
                c->last_updated = time;
                c->pts_drift = c->pts - time;
            };

            auto compute_target_delay = [&](double delay, Clock *vidclk, Clock *audclk)->double{
                //LOG_INFO("video: V - A = %f - %f\n", get_clock(vidclk), get_clock(audclk));

                auto diff = get_clock(vidclk) - get_clock(audclk);

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
                auto sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
                if(fabs(diff) < 10.0){
                    if(diff <= -sync_threshold)
                        delay = std::max(0.0,delay + diff);
                    else if(diff >= sync_threshold && delay > AV_SYNC_THRESHOLD_MAX)
                        delay += diff;
                    else if(diff >= sync_threshold)
                        delay *= 2;
                }


                LOG_INFO("video: delay=%0.3f A-V=%f\n", delay, -diff);

                return delay;
            };

            if (videoState->video_reader->NbRemaining() > 0){
                retry:
                auto lastvp = videoState->video_reader->PeekLast();
                auto vp = videoState->video_reader->PeekCur();

                // 第一帧时刻
                //if(lastvp->pts == vp->pts)
                //    videoState.frame_timer = av_gettime_relative() / 1000000.0;

                auto last_duration = vp->pts - lastvp->pts;
                auto delay = compute_target_delay(last_duration,&videoState->vidclk,&videoState->audclk);


                auto time= av_gettime_relative()/1000000.0;
                // 当前帧播放时刻(is->frame_timer+delay)大于当前时刻(time)，表示播放时刻未到
                if(time < videoState->frame_timer + delay){
                    LOG_WARNING("hecc-- 1");
                    goto display;
                }

                videoState->frame_timer += delay;
                // 校正frame_timer值：若frame_timer落后于当前系统时间太久(超过最大同步域值)，则更新为当前系统时间
                if (delay > 0 && time - videoState->frame_timer > AV_SYNC_THRESHOLD_MAX){
                    LOG_WARNING("hecc-- 2");

                    videoState->frame_timer = time;
                }

                // 更新视频时钟：时间戳、时钟时间
                set_clock(&videoState->vidclk, vp->pts);

                // 是否要丢弃未能及时播放的视频帧
                if (videoState->video_reader->NbRemaining() > 1) {
                    auto nextvp = videoState->video_reader->PeekNext();
                    auto duration = nextvp->pts - vp->pts;

                    if(time > videoState->frame_timer + duration){
                        LOG_WARNING("hecc-- 3");

                        videoState->video_reader->Next();
                        goto retry;
                    }
                }

                videoState->video_reader->Next();

            }
            LOG_WARNING("hecc-- 4");

            display:
             return videoState->video_reader->PeekLast();
        }

        void render_video(SDL_Window *mainwindow) {

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

            auto size = videoState->video_reader->GetSize();
            auto vparam = videoState->video_reader->GetParam();

            auto vertices = fitIn(WINDOW_WIGTH, WINDOW_HEIGTH, size.width, size.height);

            auto program = make_up<avf::render::GLProgram>(_vertex_shader, _fragment_shader);

            int indices[] = {
                    0, 1, 2,
                    0, 2, 3,
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

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
            glEnableVertexAttribArray(0);

            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
            glEnableVertexAttribArray(1);

            auto vframe = make_sp<avf::codec::AVVideoFrame>(size.width, size.height);

            SDL_Event event;
            bool quit = false;
            checkGLError(__LINE__);

            while (!quit) {

                glClearColor(0.2f, 0.3f, 0.3f, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);

                auto f = refresh_video();
                //LOG_INFO("peek frame:[%dx%d] -- %lf", f->width, f->height, f->pts);


                vframe->reset();

                vframe->Convert(f->frame->data, f->frame->linesize,
                                (avf::codec::VFrameFmt) vparam.pix_fmt);

                auto texture = make_up<avf::render::GLTexture>(vframe->GetWidth(), vframe->GetHeight(),
                                                                 vframe->get());

                program->Use();

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture->GetTextureId());

                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                checkGLError(__LINE__);

                SDL_GL_SwapWindow(mainwindow);

                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        quit = true;
                    }
                }
                SDL_Delay(1);
            }

        }

        void playVideo() {

            SDL_Window *mainwindow = nullptr;
            SDL_GLContext maincontext;

            initGLContext(&mainwindow, &maincontext);

            render_video(mainwindow);

        }

        void wait() {
            _th_devid.join();
            _th_deaud.join();
            _th_demux.join();

            _th_play_aud.join();
        }
    };


    AVPlayer::AVPlayer() : _impl(make_up<Impl>()) {}

    AVPlayer::~AVPlayer() {}

    void AVPlayer::OpenVideo(std::string_view filename) {
        _impl->open_video(filename);
    }

    void AVPlayer::CloseVideo() {
        _impl->close_video();
    }

    void AVPlayer::Start() {

        _impl->playAudio();

        _impl->playVideo();


        while (1) {
            LOG_INFO("start ... ...");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }


    }

    void AVPlayer::Pause() {

    }

    void AVPlayer::Seek(int64_t time_ms) {

    }
}