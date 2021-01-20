#ifndef AVAudioResample_h__
#define AVAudioResample_h__

#include "AVCommon.h"
#include "AVAudioFrame.h"

namespace avfun
{
	namespace codec
	{
		struct AVAudioStruct
		{
			uint8_t** data;
			int sample_rate;
			int channels;
			int sample_fmt;
			int nb_samples;
		};

		class AVAudioResample
		{
		public:
			AVAudioResample();
			
			~AVAudioResample();

			void Setup(SP<AVAudioStruct> audioSt);
			SP<AVAudioFrame> Resample(SP<AVAudioStruct> frameData);

		private:
			struct Impl;
			UP<Impl> _impl;
		};
	}
}

#endif // AVAudioResample_h__
