#include "AVPlayer.h"

#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <array>

#include <SDL.h>

#include "glad/glad.h"

#include "codec/ffmpeg_config.h"
#include "LogUtil.h"
#include "render/GLProgram.h"
#include "render/GLTexture.h"
#include "codec/AVVideoFrame.h"

using namespace std::chrono_literals;

namespace avf {

    typedef struct AVFClock {
        double pts {0};           /* clock base */
        double pts_drift{0};     /* clock base minus time at which we updated the clock */
        double last_updated{0};
        int serial{-1};           /* clock is based on a packet with this serial */
        int paused{0};
    } Clock;

    struct AVFPacketNode {
        AVPacket *packet;
        AVFPacketNode *next;
    };

#define PACKET_QUEUE_SIZE 4 * 1024 * 1024

    struct AVFPacketQueue {
        AVFPacketNode *first, *last;
        int nb_packets{0};
        int size{0};

        std::condition_variable cv_wb;
        std::mutex mtx;

        std::condition_variable cv_rb;


        void push(AVPacket *packet) {
            std::unique_lock<std::mutex> lock(mtx);

            if (size > PACKET_QUEUE_SIZE) {
                cv_wb.wait(lock);
            }

            AVFPacketNode *node = (AVFPacketNode *) malloc(sizeof(AVFPacketNode));
            node->packet = av_packet_alloc();
            node->next = nullptr;
            av_packet_move_ref(node->packet, packet);

            if (!first) {
                first = node;
            } else {
                last->next = node;
            }
            last = node;

            nb_packets++;

            size += sizeof(AVFPacketNode) + node->packet->size;

            //LOG_INFO("push pkg: [%d] %d -- %d | %lld", node->packet->stream_index, nb_packets, size,node->packet->pts);

            cv_rb.notify_one();

        }

        void peek(AVPacket *packet) {
            std::unique_lock<std::mutex> lock(mtx);

            if (nb_packets == 0 || size == 0) {
                cv_rb.wait(lock);
            }

            AVFPacketNode *node = first;

            first = node->next;
            if (!first) {
                last = nullptr;
            }

            av_packet_move_ref(packet, node->packet);

            av_packet_free(&node->packet);
            free(node);

            nb_packets--;
            size -= sizeof(AVFPacketNode) + packet->size;

            //LOG_INFO("peek pkg: [%d] %d -- %d | %lld", packet->stream_index, nb_packets, size, packet->pts);

            cv_wb.notify_one();
        }
    };


    struct AVFFrame {
        AVFrame *frame;
        double pts;
        int width;
        int height;
        int format;
    };

    template<size_t _Size>
    struct AVFFrameQueue {

        AVFFrameQueue() {

            for (int i = 0; i < _Size; ++i) {
                queue[i].frame = av_frame_alloc();
            }
        }

        ~AVFFrameQueue() {
            for (int i = 0; i < _Size; ++i) {
                av_frame_free(&queue[i].frame);
            }
        }

        AVFFrame queue[_Size];
        int rindex{0};
        int rindex_shown{0};
        int windex{0};
        int size{0};

        std::condition_variable cv_rb;
        std::condition_variable cv_wb;
        std::mutex mtx;

        void push(AVFrame *frame, double pts) {
            {
                std::unique_lock<std::mutex> lock(mtx);

                if (size == _Size) {
                    cv_wb.wait(lock);
                }
            }

            auto f = &queue[windex];
            f->pts = pts;
            f->width = frame->width;
            f->height = frame->height;
            f->format = frame->format;

            av_frame_move_ref(f->frame, frame);
            //LOG_INFO("push frame:[%dx%d]  %d -- %lf", f->width,f->height, size,f->pts);

            windex++;
            if (windex == _Size) {
                windex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size++;
            }

            cv_rb.notify_one();
        }

        void peek(AVFrame *frame) {
            {
                std::unique_lock<std::mutex> lock(mtx);

                if (size - rindex_shown == 0) {
                    cv_rb.wait(lock);
                }
            }

            //LOG_INFO("peek frame:[%dx%d] -- %lf", queue[rindex].width, queue[rindex].height, queue[rindex].pts);

            av_frame_move_ref(frame, queue[rindex].frame);
            av_frame_unref(queue[rindex].frame);


            rindex++;
            if (rindex == _Size) {
                rindex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size--;
            }

            cv_wb.notify_one();
        }

        AVFFrame* peek_last() {
            return &queue[rindex];
        }

        AVFFrame* peek_cur() {
            return &queue[(rindex + rindex_shown) % _Size];
        }

        AVFFrame* peek_next() {
            return &queue[(rindex + rindex_shown + 1) % _Size];
        }

        int nb_remaining(){
            return size - rindex_shown;
        }

        void next(){
            // 保留一帧
            if(!rindex_shown){
                rindex_shown = 1;
                return;
            }

            av_frame_unref(queue[rindex].frame);

            rindex++;
            if (rindex == _Size) {
                rindex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size--;
            }

            cv_wb.notify_one();

        }

    };


    struct VideoState {

        AVFPacketQueue queue_pkt_video;
        AVFPacketQueue queue_pkt_audio;

        AVFFrameQueue<3> queue_frame_video;
        AVFFrameQueue<9> queue_frame_audio;

        int width;
        int height;
        AVPixelFormat pix_fmt;
        AVFrame *video_frame;
        uint8_t *video_dst_data[4] = {NULL};
        int video_dst_linesize[4];

        AVFrame *audio_frame;
        uint8_t *audio_buf{NULL};
        int audio_buf_size{0};
        double audio_pts;

        double frame_timer{0.0};
        AVFClock vidclk;
        AVFClock audclk;

    };

    struct AVPlayer::Impl {
        AVFormatContext *fmt_ctx{nullptr};
        AVCodecContext *video_dec_ctx{nullptr};
        AVCodecContext *audio_dec_ctx{nullptr};
        int video_stream_idx;
        int audio_stream_idx;
        AVStream *video_stream{nullptr};
        AVStream *audio_stream{nullptr};

        VideoState videoState;

        std::thread _th_demux;
        std::thread _th_devid;
        std::thread _th_deaud;
        std::thread _th_play_aud;


        //////////////////////////////////////////////////////////////////////////

        int decode_packet(AVCodecContext *dec_ctx, AVFPacketQueue *queue, AVPacket *packet, AVFrame *frame) {

            int ret = 0;

            for (;;) {

                do {

                    ret = avcodec_receive_frame(dec_ctx, frame);
                    //LOG_INFO("## avcodec_receive_frame: %d",ret);
                    //LOG_INFO("## avcodec_receive_frame: [%dx%d] -- %lld",frame->width,frame->height,frame->pts);

                    if (ret == 0) {
                        return 0;
                    }

                    if (ret == AVERROR_EOF) {
                        LOG_ERROR("## eof");
                        return 1;
                    }

                } while (ret != AVERROR(EAGAIN));

                {
                    queue->peek(packet);
                    ret = avcodec_send_packet(dec_ctx, packet);

                    av_packet_unref(packet);

                    if (ret < 0) {
                        LOG_ERROR("Error submitting a packet for decoding (%d)", ret);
                        return -1;
                    }
                }
            }

        };

        void deocde_video() {
            LOG_INFO("## deocde_video");

            AVPacket *packet = av_packet_alloc();
            AVFrame *frame = av_frame_alloc();

            while (true) {

                auto ret = decode_packet(video_dec_ctx, &videoState.queue_pkt_video, packet, frame);
                //LOG_INFO("## decode_packet: [%dx%d] -- %lld",frame->width,frame->height,frame->pts);

                if (!ret) {
                    double pts = frame->pts * av_q2d(video_stream->time_base);
                    videoState.queue_frame_video.push(frame, pts);
                    av_frame_unref(frame);
                }
            }

            av_packet_free(&packet);
            av_frame_free(&frame);

        }

        void deocde_audio() {
            LOG_INFO("deocde_audio");

            AVPacket *packet = av_packet_alloc();
            AVFrame *frame = av_frame_alloc();


            while (true) {

                auto ret = decode_packet(audio_dec_ctx, &videoState.queue_pkt_audio, packet, frame);
                //LOG_INFO("## decode_packet: [%dx%d] -- %lld",frame->width,frame->height,frame->pts);

                if (!ret) {
                    double pts = frame->pts * av_q2d(audio_stream->time_base);
                    videoState.queue_frame_audio.push(frame, pts);
                    av_frame_unref(frame);
                }
            }

            av_packet_free(&packet);
            av_frame_free(&frame);
        }

        void read_packet() {
            LOG_INFO("read_packet");

            while (true) {
                AVPacket *pkt = av_packet_alloc();
                auto res = av_read_frame(fmt_ctx, pkt);
                if (res != 0) {
                    LOG_ERROR("failed to av_read_frame: %lld", res);
                    av_packet_free(&pkt);
                    break;
                }

                if (pkt->stream_index == video_stream_idx) {
                    videoState.queue_pkt_video.push(pkt);
                } else if (pkt->stream_index == audio_stream_idx) {
                    videoState.queue_pkt_audio.push(pkt);
                }

                av_packet_free(&pkt);
            }

        }

        int open_codec_context(int *stream_idx,
                               AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type) {
            int ret, stream_index;
            AVStream *st;
            AVDictionary *opts = NULL;

            ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
            if (ret < 0) {
                LOG_ERROR("Could not find %s stream", av_get_media_type_string(type));
                return ret;
            } else {
                stream_index = ret;
                st = fmt_ctx->streams[stream_index];

                /* find decoder for the stream */
                auto dec = avcodec_find_decoder(st->codecpar->codec_id);
                if (!dec) {
                    LOG_ERROR("Failed to find %s codec", av_get_media_type_string(type));
                    return AVERROR(EINVAL);
                }

                /* Allocate a codec context for the decoder */
                *dec_ctx = avcodec_alloc_context3(dec);
                if (!*dec_ctx) {
                    LOG_ERROR("Failed to allocate the %s codec context", av_get_media_type_string(type));
                    return AVERROR(ENOMEM);
                }

                /* Copy codec parameters from input stream to output codec context */
                if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
                    LOG_ERROR("Failed to copy %s codec parameters to decoder context", av_get_media_type_string(type));
                    return ret;
                }

                /* Init the decoders */
                if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
                    LOG_ERROR("Failed to open %s codec", av_get_media_type_string(type));
                    return ret;
                }
                *stream_idx = stream_index;
            }

            videoState.audio_frame = av_frame_alloc();
            videoState.video_frame = av_frame_alloc();

            return 0;
        }

        void demux(std::string_view filename) {

            /* open input file, and allocate format context */
            if (avformat_open_input(&fmt_ctx, filename.data(), NULL, NULL) < 0) {
                LOG_ERROR("Could not open source file %s", filename.data());
                exit(1);
            }

            /* retrieve stream information */
            if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
                LOG_ERROR("Could not find stream information");
                exit(1);
            }

            /* dump input information to stderr */
            av_dump_format(fmt_ctx, 0, filename.data(), 0);


            if (fmt_ctx == NULL)return;


            if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
                video_stream = fmt_ctx->streams[video_stream_idx];

                videoState.width = video_dec_ctx->width;
                videoState.height = video_dec_ctx->height;
                videoState.pix_fmt = video_dec_ctx->pix_fmt;

                int ret = av_image_alloc(videoState.video_dst_data, videoState.video_dst_linesize,
                                         videoState.width, videoState.height, videoState.pix_fmt, 1);

                AV_Assert(ret > 0);
            }


            if (!video_stream) {
                LOG_ERROR("Could not find video stream in the input, aborting");
            } else {
                _th_devid = std::thread(&AVPlayer::Impl::deocde_video, this);
            }


            if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
                audio_stream = fmt_ctx->streams[audio_stream_idx];
            }


            if (!audio_stream) {
                LOG_ERROR("Could not find audio stream in the input, aborting");
            } else {
                _th_deaud = std::thread(&AVPlayer::Impl::deocde_audio, this);
            }

            read_packet();

        }

        void openVideo(std::string_view filename) {

            //解复用线程
            std::thread th(&AVPlayer::Impl::demux, this, filename);

            _th_demux = std::move(th);
        }

        static void fill_audio(void *udata, Uint8 *stream, int len) {
            VideoState *vs = (VideoState *) udata;

            double audio_callback_time = av_gettime_relative() / 1000000.0;

            memset(stream, 0, len);

            while (len > 0) {
                if (vs->audio_buf_size == 0) {
                    av_frame_unref(vs->audio_frame);
                    vs->audio_pts = vs->queue_frame_audio.peek_cur()->pts;
                    vs->queue_frame_audio.peek(vs->audio_frame);

                    vs->audio_buf = vs->audio_frame->data[0];
                    vs->audio_buf_size = vs->audio_frame->linesize[0] / 2;
                }

                int len1 = len > vs->audio_buf_size ? vs->audio_buf_size : len;
                memcpy(stream, vs->audio_buf, len1);

                vs->audio_buf += len1;
                vs->audio_buf_size -= len1;

                stream += len1;
                len -= len1;
            }

            auto set_clock_at = [](AVFClock *c, double pts,double time)
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
            wanted.freq = 44100;
            wanted.format = AUDIO_F32;
            wanted.channels = 1;    /* 1 = mono, 2 = stereo */
            wanted.samples = 1024;  /* Good low-latency value for callback */
            wanted.callback = AVPlayer::Impl::fill_audio;
            wanted.userdata = &videoState;

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

        AVFFrame* refresh_video(){

            auto get_clock = [](AVFClock *c)->double{
                if (c->paused) {
                    return c->pts;
                } else {
                    double time = av_gettime_relative() / 1000000.0;
                    return c->pts_drift + time;
                }
            };

            auto set_clock = [](AVFClock *c, double pts)
            {
                double time = av_gettime_relative() / 1000000.0;

                c->pts = pts;
                c->last_updated = time;
                c->pts_drift = c->pts - time;
            };

            auto compute_target_delay = [&](double delay,AVFClock *vidclk,AVFClock *audclk)->double{
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

            if (videoState.queue_frame_video.nb_remaining() > 0){
                retry:
                auto lastvp = videoState.queue_frame_video.peek_last();
                auto vp = videoState.queue_frame_video.peek_cur();

                // 第一帧时刻
                //if(lastvp->pts == vp->pts)
                //    videoState.frame_timer = av_gettime_relative() / 1000000.0;

                auto last_duration = vp->pts - lastvp->pts;
                auto delay = compute_target_delay(last_duration,&videoState.vidclk,&videoState.audclk);


                auto time= av_gettime_relative()/1000000.0;
                // 当前帧播放时刻(is->frame_timer+delay)大于当前时刻(time)，表示播放时刻未到
                if(time < videoState.frame_timer + delay){
                    goto display;
                }

                videoState.frame_timer += delay;
                // 校正frame_timer值：若frame_timer落后于当前系统时间太久(超过最大同步域值)，则更新为当前系统时间
                if (delay > 0 && time - videoState.frame_timer > AV_SYNC_THRESHOLD_MAX)
                    videoState.frame_timer = time;

                // 更新视频时钟：时间戳、时钟时间
                set_clock(&videoState.vidclk, vp->pts);

                // 是否要丢弃未能及时播放的视频帧
                if (videoState.queue_frame_video.nb_remaining() > 1) {
                    auto nextvp = videoState.queue_frame_video.peek_next();
                    auto duration = nextvp->pts - vp->pts;

                    if(time > videoState.frame_timer + duration){
                        videoState.queue_frame_video.next();
                        goto retry;
                    }
                }

                videoState.queue_frame_video.next();

            }

            display:
             return videoState.queue_frame_video.peek_last();
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


            auto vertices = fitIn(WINDOW_WIGTH, WINDOW_HEIGTH, videoState.width, videoState.height);

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

            auto vframe = make_sp<avf::codec::AVVideoFrame>(videoState.width, videoState.height);

            SDL_Event event;
            bool quit = false;
            checkGLError(__LINE__);

            while (!quit) {

                glClearColor(0.2f, 0.3f, 0.3f, 1.0);
                glClear(GL_COLOR_BUFFER_BIT);


                //auto frame = videoState.video_frame;
                //av_frame_unref(frame);

                //videoState.frame_queue.peek(frame);
                auto f = refresh_video();
                //LOG_INFO("peek frame:[%dx%d] -- %lf", f->width, f->height, f->pts);


                av_image_copy(videoState.video_dst_data, videoState.video_dst_linesize,
                              (const uint8_t **) (f->frame->data), f->frame->linesize,
                              videoState.pix_fmt, videoState.width, videoState.height);

                vframe->reset();

                vframe->Convert(videoState.video_dst_data, videoState.video_dst_linesize,
                                (avf::codec::VFrameFmt) videoState.pix_fmt);

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
        _impl->openVideo(filename);
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