#include "AVAudioBuffer.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"

namespace avfun
{
	namespace codec
	{
		struct AVAudioBuffer::Impl
		{
			AVAudioFifo* fifo{ NULL };

			void init();
			void add_samples(uint8_t** input_samples, int nb_samples);
			SP<AVAudioFrame> read_samples();
			int buf_size();
			void release();
		};

		void AVAudioBuffer::Impl::init() {
			/* Create the FIFO buffer based on the specified output sample format. */
			if (!(fifo = av_audio_fifo_alloc(SUPPORT_AUDIO_SAMPLE_FMT, SUPPORT_AUDIO_CHANNELS, 1))) {
				LOG_ERROR("Could not allocate FIFO");
			}
		}

		void AVAudioBuffer::Impl::add_samples(uint8_t** input_samples, int nb_samples) {
			int error;

			/* Make the FIFO as large as it needs to be to hold both,
			 * the old and the new samples. */
			if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + nb_samples)) < 0) {
				LOG_ERROR("Could not reallocate FIFO");
				return;
			}

			/* Store the new samples in the FIFO buffer. */
			if (av_audio_fifo_write(fifo, (void**)input_samples,
				nb_samples) < nb_samples) {
				LOG_ERROR("Could not write data to FIFO");
				return;
			}
		}

		SP<AVAudioFrame> AVAudioBuffer::Impl::read_samples() {

			int frame_size = FFMIN(av_audio_fifo_size(fifo), SUPPORT_AUDIO_SAMPLE_NUM);

			auto aframe = make_sp<AVAudioFrame>(frame_size);

			if (av_audio_fifo_read(fifo, (void**)aframe->get(), frame_size) < frame_size) {
				LOG_ERROR("Could not read data from FIFO");
				return nullptr;
			}

			return aframe;
		}

		int AVAudioBuffer::Impl::buf_size() {
			return av_audio_fifo_size(fifo);
		}

		void AVAudioBuffer::Impl::release() {
			if (fifo)
				av_audio_fifo_free(fifo);
		}

		AVAudioBuffer::AVAudioBuffer():_impl(make_up<AVAudioBuffer::Impl>()) {
			_impl->init();
		}

		AVAudioBuffer::~AVAudioBuffer() {

		}

		void AVAudioBuffer::AddSamples(uint8_t** input_samples, int nb_samples) {
			return _impl->add_samples(input_samples, nb_samples);
		}

		SP<AVAudioFrame> AVAudioBuffer::ReadFrame() {
			return _impl->read_samples();
		}

		int AVAudioBuffer::GetBufSize() {
			return _impl->buf_size();
		}

	}
}
