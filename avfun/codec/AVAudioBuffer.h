#ifndef AVAudioBuffer_h__
#define AVAudioBuffer_h__

#include "AVCommon.h"
#include "AVAudioFrame.h"

namespace avfun
{
	namespace codec
	{
		class AVAudioBuffer
		{
		public:
			AVAudioBuffer();
			~AVAudioBuffer();
			void AddSamples(uint8_t** input_samples, int nb_samples);
			SP<AVAudioFrame> ReadFrame();
			int GetBufSize();
		private:
			struct Impl;
			UP<Impl> _impl;
		};
	}
}

#endif // AVAudioBuffer_h__
