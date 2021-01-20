#ifndef AVAudioReader_h__
#define AVAudioReader_h__

#include "AVCommon.h"

namespace avfun
{
	namespace codec
	{
		class AVAudioFrame;

		class AVAudioReader
		{
		public:

			static UP<AVAudioReader> Make(std::string_view filename);

			virtual void SetupDecoder() = 0;
			virtual SP<AVAudioFrame>  ReadNextFrame() = 0;
		};

	}
}

#endif // AVAudioReader_h__
