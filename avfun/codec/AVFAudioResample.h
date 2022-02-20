#ifndef AVFUN_AVFAUDIORESAMPLE_H
#define AVFUN_AVFAUDIORESAMPLE_H

#include "AVCommon.h"

namespace avf
{
	namespace codec
	{
        class AudioParam;
        class AudioFrame;

		class AudioResample
		{
		public:
			AudioResample();
			
			~AudioResample();

			void Setup(const AudioParam& info);
			int Convert(SP<AudioFrame> in, SP<AudioFrame> out);
			uint8_t** data();

		private:
			struct Impl;
			UP<Impl> _impl;
		};
	}
}

#endif // AVFUN_AVFAUDIORESAMPLE_H
