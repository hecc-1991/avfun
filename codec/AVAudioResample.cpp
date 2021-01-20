#include "AVAudioResample.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"

namespace avfun
{
	namespace codec
	{

#define SUPPORT_AUDIO_SAMPLE_RATE	44100
#define SUPPORT_AUDIO_CHANNEL		AV_CH_LAYOUT_STEREO
#define SUPPORT_AUDIO_SAMPLE_FMT	AV_SAMPLE_FMT_S16
#define SUPPORT_AUDIO_SAMPLE_NUM	1024

		struct AVAudioResample::Impl
		{
			struct SwrContext* swr_ctx;

			void init(SP<AVAudioStruct> audioSt);
			SP<AVAudioFrame> resample(SP<AVAudioStruct> frameData);
			void release();

		}; 

		void AVAudioResample::Impl::init(SP<AVAudioStruct> audioSt) {
			int ret;
			swr_ctx = swr_alloc();
			if (!swr_ctx) {
				LOG_ERROR("Could not allocate resampler context");
				return;
			}

			/* set options */
			av_opt_set_int(swr_ctx, "in_channel_layout", audioSt->channels, 0);
			av_opt_set_int(swr_ctx, "in_sample_rate", audioSt->sample_rate, 0);
			av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (enum AVSampleFormat)audioSt->sample_fmt, 0);

			av_opt_set_int(swr_ctx, "out_channel_layout", SUPPORT_AUDIO_CHANNEL, 0);
			av_opt_set_int(swr_ctx, "out_sample_rate", SUPPORT_AUDIO_SAMPLE_RATE, 0);
			av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", SUPPORT_AUDIO_SAMPLE_FMT, 0);

			/* initialize the resampling context */
			if (swr_init(swr_ctx) < 0) {
				LOG_ERROR("Failed to initialize the resampling context");
				return;
			}

		}

		SP<AVAudioFrame> AVAudioResample::Impl::resample(SP<AVAudioStruct> audioSt) {

			auto aframe = make_sp<AVAudioFrame>();
			
			int dst_linesize;

			/* compute the number of converted samples: buffering is avoided
			* ensuring that the output buffer will contain at least all the
			* converted input samples */
			int max_dst_nb_samples, dst_nb_samples;
			max_dst_nb_samples = dst_nb_samples =
				av_rescale_rnd(audioSt->nb_samples, SUPPORT_AUDIO_SAMPLE_RATE, audioSt->sample_rate, AV_ROUND_UP);

			/* buffer is going to be directly written to a rawaudio file, no alignment */
			int dst_nb_channels = av_get_channel_layout_nb_channels(SUPPORT_AUDIO_CHANNEL);
			int ret = av_samples_alloc_array_and_samples(aframe->getAddr(), &dst_linesize, dst_nb_channels,
				dst_nb_samples, SUPPORT_AUDIO_SAMPLE_FMT, 0);
			if (ret < 0) {
				LOG_ERROR("Could not allocate destination samples");
				goto end;
			}

			/* compute destination number of samples */
			dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, audioSt->sample_rate) +
				audioSt->nb_samples, SUPPORT_AUDIO_SAMPLE_RATE, audioSt->sample_rate, AV_ROUND_UP);
			if (dst_nb_samples > max_dst_nb_samples) {
				av_freep(&aframe->get()[0]);
				ret = av_samples_alloc(aframe->get(), &dst_linesize, dst_nb_channels,
					dst_nb_samples, SUPPORT_AUDIO_SAMPLE_FMT, 1);
				if (ret < 0)
					LOG_ERROR("Could not allocate destination samples");
				max_dst_nb_samples = dst_nb_samples;
			}

			/* convert to destination format */
			ret = swr_convert(swr_ctx, aframe->get(), dst_nb_samples, (const uint8_t**)audioSt->data, audioSt->nb_samples);

			if (ret < 0) {
				LOG_ERROR("Error while converting");
				goto end;
			}

			aframe->SetLinesize(dst_linesize);

			int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
				ret, SUPPORT_AUDIO_SAMPLE_FMT, 1);
			if (dst_bufsize < 0) {
				LOG_ERROR("Could not get sample buffer size");
				goto end;
			}

			LOG_INFO("in:%d out:%d", audioSt->nb_samples, ret);

		end:
			return aframe;

		}

		void AVAudioResample::Impl::release() {
			swr_free(&swr_ctx);
		}


		AVAudioResample::AVAudioResample() :_impl(make_up<Impl>()){
		}

		AVAudioResample::~AVAudioResample() {
			_impl->release();
		}

		void AVAudioResample::Setup(SP<AVAudioStruct> audioSt) {
			_impl->init(audioSt);
		}

		SP<AVAudioFrame> AVAudioResample::Resample(SP<AVAudioStruct> frameData) {
			return _impl->resample(frameData);
		}

	}
}