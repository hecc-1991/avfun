#ifndef ffmpeg_config_h__
#define ffmpeg_config_h__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>

#ifdef __cplusplus
}
#endif // __cplusplus

#define SUPPORT_AUDIO_SAMPLE_RATE		44100
#define SUPPORT_AUDIO_CHANNELS			1
#define SUPPORT_AUDIO_LAYOUT			AV_CH_LAYOUT_MONO
#define SUPPORT_AUDIO_SAMPLE_FMT_UNIT	2
#define SUPPORT_AUDIO_SAMPLE_FMT		AV_SAMPLE_FMT_S16
#define SUPPORT_AUDIO_SAMPLE_NUM		1024

#endif // ffmpeg_config_h__
