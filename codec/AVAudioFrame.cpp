#include "AVAudioFrame.h"
#include "LogUtil.h"

#include "ffmpeg_config.h"

namespace avfun
{
	namespace codec
	{
		struct AVAudioFrame::Impl {

			AVFrame* frame;

            int init_frame(int frame_size);
			void release();
		};

         int AVAudioFrame::Impl::init_frame( int frame_size)
        {
            int error;

            /* Create a new frame to store the audio samples. */
            if (!(frame = av_frame_alloc())) {
				LOG_ERROR("Could not allocate output frame");
                return AVERROR_EXIT;
            }

			frame->nb_samples = frame_size;
			frame->channel_layout = SUPPORT_AUDIO_LAYOUT;
			frame->format = SUPPORT_AUDIO_SAMPLE_FMT;
			frame->sample_rate = SUPPORT_AUDIO_SAMPLE_RATE;

            /* Allocate the samples of the created frame. This call will make
             * sure that the audio frame can hold as many samples as specified. */
            if ((error = av_frame_get_buffer(frame, 0)) < 0) {
                LOG_ERROR("Could not allocate output frame samples (error '%d')",
                    error);
                return error;
            }

            return 0;
        }

		 void AVAudioFrame::Impl::release() {
			 av_frame_free(&frame);
		 }

		AVAudioFrame::AVAudioFrame(int frame_size):
			_impl(make_up<AVAudioFrame::Impl>()){
			_impl->init_frame(frame_size);
		}

		AVAudioFrame::~AVAudioFrame() {
			_impl->release();
		}

		uint8_t** AVAudioFrame::get() {
			return _impl->frame->extended_data;
		}

		int AVAudioFrame::GetSize() {
			return _impl->frame->nb_samples * SUPPORT_AUDIO_CHANNELS * SUPPORT_AUDIO_SAMPLE_FMT_UNIT;
		}

	}
}
