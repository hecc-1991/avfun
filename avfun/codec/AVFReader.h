#ifndef AVFUN_AVFREADER_H
#define AVFUN_AVFREADER_H

#include "ffmpeg_config.h"

#define AVF_TIME_BASE 1000

class Reader {
public:

    void open(std::string_view filename,AVFormatContext** fmt_ctx) {
        /* open input file, and allocate format context */
        if (avformat_open_input(fmt_ctx, filename.data(), NULL, NULL) < 0) {
            LOG_ERROR("Could not open source file %s", filename.data());
            exit(1);
        }

        /* retrieve stream information */
        if (avformat_find_stream_info(*fmt_ctx, NULL) < 0) {
            LOG_ERROR("Could not find stream information");
            exit(1);
        }

        /* dump input information to stderr */
        av_dump_format(*fmt_ctx, 0, filename.data(), 0);
    }

    int open_codec_context(int* stream_idx, AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
    {
        int ret, stream_index;
        AVStream* st;
        AVDictionary* opts = NULL;

        ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
        if (ret < 0) {
            LOG_ERROR("Could not find %s stream",av_get_media_type_string(type));
            return ret;
        }
        else {
            stream_index = ret;
            st = fmt_ctx->streams[stream_index];

            /* find decoder for the stream */
            auto dec = avcodec_find_decoder(st->codecpar->codec_id);
            if (!dec) {
                LOG_ERROR("Failed to find %s codec",av_get_media_type_string(type));
                return AVERROR(EINVAL);
            }

            /* Allocate a codec context for the decoder */
            *dec_ctx = avcodec_alloc_context3(dec);
            if (!*dec_ctx) {
                LOG_ERROR("Failed to allocate the %s codec context",av_get_media_type_string(type));
                return AVERROR(ENOMEM);
            }

            /* Copy codec parameters from input stream to output codec context */
            if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
                LOG_ERROR("Failed to copy %s codec parameters to decoder context",av_get_media_type_string(type));
                return ret;
            }

            /* Init the decoders */
            if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
                LOG_ERROR("Failed to open %s codec",av_get_media_type_string(type));
                return ret;
            }
            *stream_idx = stream_index;
        }

        return 0;
    }
};


#endif
