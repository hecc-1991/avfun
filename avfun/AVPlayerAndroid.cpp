#include "AVPlayer.h"

#include <thread>
#include <mutex>
#include <algorithm>
#include <chrono>

#include "glad/glad.h"

#include "codec/ffmpeg_config.h"
#include "codec/AVFVideoReader.h"
#include "codec/AVFAudioReader.h"

#include "LogUtil.h"
#include "Utils.h"
#include "codec/AVVideoFrame.h"

using namespace std::chrono_literals;
using namespace avf::codec;

namespace avf {

#define SEEK_AUDIO  1
#define SEEK_VIDEO  2

    typedef struct Clock {
        double pts{0};           /* clock base */
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

        std::atomic_bool paused{false};

        int64_t seek_pos{0};
        std::atomic_int seek_req{0};

    };

    struct AVPlayer::Impl {

        VideoState *videoState;

        std::thread _th_play_aud;

        //////////////////////////////////////////////////////////////////////////

        void init_clock(Clock *c) {
            double time = Utils::GetTimeSec();
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

        }

        void close_video() {
            videoState = new VideoState();
        }

        void play_audio() {
        }

        void playAudio() {
            std::thread th(&AVPlayer::Impl::play_audio, this);

            _th_play_aud = std::move(th);
        }

        PicFrame *refresh_video() {

            auto get_clock = [](Clock *c) -> double {
                if (c->paused) {
                    return c->pts;
                } else {
                    double time = Utils::GetTimeSec();
                    return c->pts_drift + time;
                }
            };

            auto set_clock = [](Clock *c, double pts) {
                double time = Utils::GetTimeSec();

                c->pts = pts;
                c->last_updated = time;
                c->pts_drift = c->pts - time;
            };

            auto compute_target_delay = [&](double delay, Clock *vidclk, Clock *audclk) -> double {
//                LOG_INFO("video: V - A = %f - %f\n", get_clock(vidclk), get_clock(audclk));

                auto diff = get_clock(vidclk) - get_clock(audclk);

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1
                auto sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
                if (fabs(diff) < 3600.0) {
                    if (diff <= -sync_threshold)
                        delay = std::max(0.0, delay + diff);
                    else if (diff >= sync_threshold && delay > AV_SYNC_THRESHOLD_MAX)
                        delay += diff;
                    else if (diff >= sync_threshold)
                        delay *= 2;
                }

//                LOG_INFO("video: delay=%0.3f A-V=%f\n", delay, -diff);

                return delay;
            };


            auto vp_duration = [&](PicFrame *vp, PicFrame *nextvp) -> double {
                if (vp->serial == nextvp->serial) {
                    auto duration = nextvp->pts - vp->pts;
                    if (duration <= 0)
                        return vp->duration;
                    return duration;
                } else {
                    return 0;
                }
            };

            retry:
            if (videoState->video_reader->NbRemaining() > 0) {
                auto lastvp = videoState->video_reader->PeekLast();
                auto vp = videoState->video_reader->PeekCur();

                if (!videoState->video_reader->Serial()) {
                    videoState->video_reader->Next();
                    goto retry;
                }

                if (lastvp->serial != vp->serial)
                    videoState->frame_timer = Utils::GetTimeSec();

                if (videoState->paused) {
                    goto display;
                }

                auto last_duration = vp_duration(lastvp, vp);
                auto delay = compute_target_delay(last_duration, &videoState->vidclk, &videoState->audclk);


                auto time = Utils::GetTimeSec();
                if (time < videoState->frame_timer + delay) {
                    goto display;
                }

                videoState->frame_timer += delay;
                if (delay > 0 && time - videoState->frame_timer > AV_SYNC_THRESHOLD_MAX) {
                    videoState->frame_timer = time;
                }

                set_clock(&videoState->vidclk, vp->pts);

                if (videoState->video_reader->NbRemaining() > 1) {
                    auto nextvp = videoState->video_reader->PeekNext();
                    auto duration = vp_duration(vp, nextvp);

                    if (time > videoState->frame_timer + duration) {
                        videoState->video_reader->Next();
                        goto retry;
                    }
                }

                videoState->video_reader->Next();

            }


            if (videoState->seek_req & SEEK_VIDEO) {
                videoState->video_reader->NextAt(videoState->seek_pos);
                videoState->seek_req -= SEEK_VIDEO;
                goto retry;
            }

        display:
            return videoState->video_reader->PeekLast();
        }

        void pause() {
            videoState->paused = !videoState->paused;
        }

        void seek(int64_t time_ms) {
            if (!videoState->seek_req) {
                videoState->seek_pos = time_ms;
                videoState->seek_req = SEEK_AUDIO + SEEK_VIDEO;
            }
        }

        void wait() {
            _th_play_aud.join();
        }

        AVFSizei getSize() {
            return videoState->video_reader->GetSize();
        }

        int getFrame(SP<avf::codec::AVVideoFrame> frame) {
            auto f = refresh_video();
            //LOG_INFO("peek frame:[%dx%d] -- %lf", f->width, f->height, f->pts);

            frame->reset();

            auto vparam = videoState->video_reader->GetParam();

            frame->Convert(f->frame->data, f->frame->linesize,
                           (avf::codec::VFrameFmt) vparam.pix_fmt);

            return 0;
        }

        double getDuration(){
            return videoState->video_reader->GetDuration();
        }

        double getProgress(){
            return videoState->audio_pts;
        }
    };


    AVPlayer::AVPlayer() : _impl(make_up<Impl>()) {
        _Stat = PLAYER_STAT::NONE;
    }

    AVPlayer::~AVPlayer() {}

    void AVPlayer::OpenVideo(std::string_view filename) {
        _impl->open_video(filename);
        _Stat = PLAYER_STAT::INIT;
    }

    void AVPlayer::CloseVideo() {
        _impl->close_video();
    }

    void AVPlayer::Play() {
        _impl->playAudio();

        _Stat = PLAYER_STAT::PLAYING;
    }

    void AVPlayer::Pause() {
        _impl->pause();

        _Stat = _Stat == PLAYER_STAT::PLAYING ? PLAYER_STAT::PAUSE : PLAYER_STAT::PLAYING;
    }

    void AVPlayer::Seek(int64_t time_ms) {
        _impl->seek(time_ms);
    }

    AVFSizei AVPlayer::GetSize() {
        return _impl->getSize();
    }

    int AVPlayer::GetFrame(SP<avf::codec::AVVideoFrame> frame) {
        return _impl->getFrame(frame);
    }

    double AVPlayer::GetDuration(){
        return _impl->getDuration();
    }

    double AVPlayer::GetProgress(){
        return _impl->getProgress();
    }


}