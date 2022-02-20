#include "AVFAudioResample.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"
#include "AVFAudioReader.h"

namespace avf
{
	namespace codec
	{
		struct AudioResample::Impl
		{
            AudioParam audio_info;
			struct SwrContext* swr_ctx;

			void init(const AudioParam& info);
			int convert(SP<AudioFrame> in, SP<AudioFrame> out);
			uint8_t** data();
			void release();

		}; 

		void AudioResample::Impl::init(const AudioParam& info) {
            audio_info = info;

			int ret;
			swr_ctx = swr_alloc();
			if (!swr_ctx) {
				LOG_ERROR("Could not allocate resampler context");
				return;
			}

			/* set options */
			av_opt_set_int(swr_ctx, "in_channel_layout", info.channels, 0);
			av_opt_set_int(swr_ctx, "in_sample_rate", info.sample_rate, 0);
			av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (enum AVSampleFormat)info.sample_fmt, 0);

			av_opt_set_int(swr_ctx, "out_channel_layout", SUPPORT_AUDIO_LAYOUT, 0);
			av_opt_set_int(swr_ctx, "out_sample_rate", SUPPORT_AUDIO_SAMPLE_RATE, 0);
			av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", SUPPORT_AUDIO_SAMPLE_FMT, 0);

			/* initialize the resampling context */
			if (swr_init(swr_ctx) < 0) {
				LOG_ERROR("Failed to initialize the resampling context");
				return;
			}

		}

		int AudioResample::Impl::convert(SP<AudioFrame> in, SP<AudioFrame> out) {

			int wanted_nb_samples = in->nb_samples;
            int out_count = (int64_t)wanted_nb_samples * SUPPORT_AUDIO_SAMPLE_RATE / audio_info.sample_rate + 256;
            int out_size  = av_samples_get_buffer_size(NULL, SUPPORT_AUDIO_CHANNELS, out_count, SUPPORT_AUDIO_SAMPLE_FMT, 0);
            AV_Assert(out_size);
            av_fast_malloc(&out->buf, &out->buf_size, out_size);
            auto len = swr_convert(swr_ctx, &out->buf, out_count, (const uint8_t**)in->data, in->nb_samples);
            AV_Assert(len);
            out->buf_size = len * SUPPORT_AUDIO_CHANNELS * av_get_bytes_per_sample(SUPPORT_AUDIO_SAMPLE_FMT);
        }

		uint8_t** AudioResample::Impl::data() {
			return nullptr;
		}

		void AudioResample::Impl::release() {
			swr_free(&swr_ctx);
		}


		AudioResample::AudioResample() : _impl(make_up<Impl>()){
		}

		AudioResample::~AudioResample() {
			_impl->release();
		}

        void AudioResample::Setup(const AudioParam& info) {
			_impl->init(info);
		}

		int AudioResample::Convert(SP<AudioFrame> in, SP<AudioFrame> out) {
			return _impl->convert(in, out);
		}

		uint8_t** AudioResample::data() {
			return _impl->data();
		}

	}
}