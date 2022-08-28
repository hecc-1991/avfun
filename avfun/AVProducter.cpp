#include "AVProducter.h"

#include <thread>

#include "codec/ffmpeg_config.h"
#include "codec/AVFVideoReader.h"
#include "codec/AVFAudioReader.h"

using namespace avf::codec;

namespace avf {

    struct AVProducter::Impl {

#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

        typedef struct OutputStream {
            AVStream *st;
            AVCodecContext *enc;

            /* pts of the next frame that will be generated */
            int64_t next_pts;
            int samples_count;

            AVPacket *tmp_pkt;

            float t, tincr, tincr2;

            struct SwsContext *sws_ctx;
            struct SwrContext *swr_ctx;
        } OutputStream;

        UP<VideoReader> video_reader;
        UP<AudioReader> audio_reader;
        std::thread th_deaud;

        AVFormatContext *fmt_ctx{nullptr};
        const AVOutputFormat *output_fmt{nullptr};
        OutputStream video_st{0};
        OutputStream audio_st{0};
        const AVCodec *audio_codec, *video_codec;
        AVDictionary *opt{NULL};

        //////////////////////////////////////////////////////////////////////////

        void setVideoSource(std::string_view filename) {
            video_reader = VideoReader::Make(filename);
        }

        void setAudioSource(std::string_view filename) {
            audio_reader = AudioReader::Make(filename);
        }

        void add_stream(OutputStream *ost, AVFormatContext *oc,
                        const AVCodec **codec,
                        enum AVCodecID codec_id) {
            AVCodecContext *c;
            int i;

            /* find the encoder */
            *codec = avcodec_find_encoder(codec_id);

            if (!(*codec)) {
                LOG_ERROR("Could not find encoder for '%s'", avcodec_get_name(codec_id));
                AV_Assert(*codec);
            }

            ost->tmp_pkt = av_packet_alloc();

            ost->st = avformat_new_stream(oc, NULL);
            AV_Assert(ost->st);

            ost->st->id = oc->nb_streams - 1;
            c = avcodec_alloc_context3(*codec);
            AV_Assert(c);

            ost->enc = c;

            switch ((*codec)->type) {
                case AVMEDIA_TYPE_AUDIO:
                    c->sample_fmt = (*codec)->sample_fmts ?
                                    (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
                    c->bit_rate = 64000;
                    c->sample_rate = 44100;
                    if ((*codec)->supported_samplerates) {
                        c->sample_rate = (*codec)->supported_samplerates[0];
                        for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                            if ((*codec)->supported_samplerates[i] == 44100)
                                c->sample_rate = 44100;
                        }
                    }
                    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
                    if ((*codec)->channel_layouts) {
                        c->channel_layout = (*codec)->channel_layouts[0];
                        for (i = 0; (*codec)->channel_layouts[i]; i++) {
                            if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                                c->channel_layout = AV_CH_LAYOUT_STEREO;
                        }
                    }
                    c->channels = av_get_channel_layout_nb_channels(c->channel_layout);
                    ost->st->time_base = (AVRational) {1, c->sample_rate};
                    break;

                case AVMEDIA_TYPE_VIDEO:
                    c->codec_id = codec_id;

                    c->bit_rate = 400000;
                    /* Resolution must be a multiple of two. */
                    c->width = 352;
                    c->height = 288;
                    /* timebase: This is the fundamental unit of time (in seconds) in terms
                     * of which frame timestamps are represented. For fixed-fps content,
                     * timebase should be 1/framerate and timestamp increments should be
                     * identical to 1. */
                    ost->st->time_base = (AVRational) {1, STREAM_FRAME_RATE};
                    c->time_base = ost->st->time_base;

                    c->gop_size = 12; /* emit one intra frame every twelve frames at most */
                    c->pix_fmt = STREAM_PIX_FMT;
                    if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                        /* just for testing, we also add B-frames */
                        c->max_b_frames = 2;
                    }
                    if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                        /* Needed to avoid using macroblocks in which some coeffs overflow.
                         * This does not happen with normal video, it just happens here as
                         * the motion of the chroma plane does not match the luma plane. */
                        c->mb_decision = 2;
                    }
                    break;

                default:
                    break;
            }

            /* Some formats want stream headers to be separate. */
            if (oc->oformat->flags & AVFMT_GLOBALHEADER)
                c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        void open_video(AVFormatContext *oc, const AVCodec *codec,
                        OutputStream *ost, AVDictionary *opt_arg) {
            int ret;
            AVCodecContext *c = ost->enc;
            AVDictionary *opt = NULL;

            av_dict_copy(&opt, opt_arg, 0);

            /* open the codec */
            ret = avcodec_open2(c, codec, &opt);
            av_dict_free(&opt);
            if (ret < 0) {
                LOG_ERROR("Could not open video codec: %s", av_err2str(ret));
                AV_Assert(ret == 0);
            }

            /* copy the stream parameters to the muxer */
            ret = avcodec_parameters_from_context(ost->st->codecpar, c);
            if (ret < 0) {
                LOG_ERROR("Could not copy the stream parameters");
                AV_Assert(ret == 0);
            }
        }

        void open_audio(AVFormatContext *oc, const AVCodec *codec,
                        OutputStream *ost, AVDictionary *opt_arg) {
            AVCodecContext *c;
            int nb_samples;
            int ret;
            AVDictionary *opt = NULL;

            c = ost->enc;

            /* open it */
            av_dict_copy(&opt, opt_arg, 0);
            ret = avcodec_open2(c, codec, &opt);
            av_dict_free(&opt);
            if (ret < 0) {
                LOG_ERROR("Could not open audio codec: %s", av_err2str(ret));
                AV_Assert(ret == 0);
            }

            /* init signal generator */
            ost->t = 0;
            ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
            /* increment frequency by 110 Hz per second */
            ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

            if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
                nb_samples = 10000;
            else
                nb_samples = c->frame_size;

            /* copy the stream parameters to the muxer */
            ret = avcodec_parameters_from_context(ost->st->codecpar, c);
            if (ret < 0) {
                LOG_ERROR("Could not copy the stream parameters");
                AV_Assert(ret == 0);
            }

            /* create resampler context */
            ost->swr_ctx = swr_alloc();
            AV_Assert(ost->swr_ctx);

            /* set options */
            av_opt_set_int(ost->swr_ctx, "in_channel_count", c->channels, 0);
            av_opt_set_int(ost->swr_ctx, "in_sample_rate", c->sample_rate, 0);
            av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
            av_opt_set_int(ost->swr_ctx, "out_channel_count", c->channels, 0);
            av_opt_set_int(ost->swr_ctx, "out_sample_rate", c->sample_rate, 0);
            av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt", c->sample_fmt, 0);

            /* initialize the resampling context */
            if ((ret = swr_init(ost->swr_ctx)) < 0) {
                LOG_ERROR("Failed to initialize the resampling context");
                AV_Assert(ret == 0);
            }
        }

         void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
        {
            AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

            printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
                   av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
                   av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
                   av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
                   pkt->stream_index);
        }

         int write_frame(AVFormatContext *fmt_ctx, AVCodecContext *c,
                               AVStream *st, AVFrame *frame, AVPacket *pkt)
        {
            int ret;

            // send the frame to the encoder
            ret = avcodec_send_frame(c, frame);
            if (ret < 0) {
                fprintf(stderr, "Error sending a frame to the encoder: %s\n",
                        av_err2str(ret));
                exit(1);
            }

            while (ret >= 0) {
                ret = avcodec_receive_packet(c, pkt);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                else if (ret < 0) {
                    fprintf(stderr, "Error encoding a frame: %s\n", av_err2str(ret));
                    exit(1);
                }

                /* rescale output packet timestamp values from codec to stream timebase */
                av_packet_rescale_ts(pkt, c->time_base, st->time_base);
                pkt->stream_index = st->index;

                /* Write the compressed frame to the media file. */
                log_packet(fmt_ctx, pkt);
                ret = av_interleaved_write_frame(fmt_ctx, pkt);
                /* pkt is now blank (av_interleaved_write_frame() takes ownership of
                 * its contents and resets pkt), so that no unreferencing is necessary.
                 * This would be different if one used av_write_frame(). */
                if (ret < 0) {
                    fprintf(stderr, "Error while writing output packet: %s\n", av_err2str(ret));
                    exit(1);
                }
            }

            return ret == AVERROR_EOF ? 1 : 0;
        }

        int setOutput(std::string_view filename) {
            int ret;

            avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, filename.data());
            AV_Assert(fmt_ctx);

            output_fmt = fmt_ctx->oformat;

            if (output_fmt->video_codec != AV_CODEC_ID_NONE) {
                add_stream(&video_st, fmt_ctx, &video_codec, output_fmt->video_codec);
            }
            if (output_fmt->audio_codec != AV_CODEC_ID_NONE) {
                add_stream(&audio_st, fmt_ctx, &audio_codec, output_fmt->audio_codec);
            }

            open_video(fmt_ctx, video_codec, &video_st, opt);

            open_audio(fmt_ctx, audio_codec, &audio_st, opt);

            av_dump_format(fmt_ctx, 0, filename.data(), 1);

            /* open the output file, if needed */
            if (!(output_fmt->flags & AVFMT_NOFILE)) {
                ret = avio_open(&fmt_ctx->pb, filename.data(), AVIO_FLAG_WRITE);
                if (ret < 0) {
                    LOG_ERROR("Could not open '%s': %s", filename, av_err2str(ret));
                    return 1;
                }
            }

            ret = avformat_write_header(fmt_ctx, &opt);
            if (ret < 0) {
                LOG_ERROR("Error occurred when opening output file: %s", av_err2str(ret));
                return 1;
            }

            int video_eof = 0;
            int audio_eof = 0;

            auto aud_param = audio_reader->FetchInfo();

            auto aud_pts = 0;
            auto vid_pts = 0;

            auto aud_frame = av_frame_alloc();

            while (video_eof && audio_eof){

                auto aframe = audio_reader->ReadNextFrame();

                aud_frame->format = aud_param.sample_fmt;
                aud_frame->channel_layout = aud_param.channel_layout;
                aud_frame->sample_rate = aud_param.sample_rate;
                aud_frame->nb_samples = aframe->nb_samples;

                aud_frame->pts = aud_pts;

                aud_pts += aframe->nb_samples;

                while (1){
                    auto vframe = video_reader->PeekCur();

                    auto vid_frame = av_frame_alloc();


                }


            }

            return 0;
        }

        void setProgressCallback(ProgressCallback func) {

        }


        int start() {

            return 0;
        }

    };


    AVProducter::AVProducter() : _impl(std::make_unique<AVProducter::Impl>()) {

    }

    AVProducter::~AVProducter() = default;

    void AVProducter::SetVideoSource(std::string_view filename) {
        _impl->setVideoSource(filename);
    }

    void AVProducter::SetAudioSource(std::string_view filename) {
        _impl->setVideoSource(filename);
    }

    int AVProducter::SetOutput(std::string_view filename) {

        return _impl->setOutput(filename);
    }

    void AVProducter::SetProgressCallback(ProgressCallback func) {
        _impl->setProgressCallback(func);
    }

    int AVProducter::Start() {
        return _impl->start();
    }
}