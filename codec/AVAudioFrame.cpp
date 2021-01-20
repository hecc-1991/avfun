#include "AVAudioFrame.h"
#include "LogUtil.h"

#include "ffmpeg_config.h"

namespace avfun
{
	namespace codec
	{

		AVAudioFrame::AVAudioFrame(){
		}

		AVAudioFrame::~AVAudioFrame() {
			if (data)
				av_freep(&data[0]);
			av_freep(&data);
		}



		uint8_t*** AVAudioFrame::getAddr() {
			return &data;
		}

		uint8_t** AVAudioFrame::get() {
			return data;
		}

		void AVAudioFrame::SetLinesize(int size) {
			linesize = size;
		}

		int AVAudioFrame::GetLinesize() {
			return linesize;
		}

	}
}
