#ifndef AVAudioFrame_h__
#define AVAudioFrame_h__

#include "AVCommon.h"

namespace avfun
{
	namespace codec
	{
		class AVAudioFrame
		{
		public:
			AVAudioFrame(int frame_size);
			~AVAudioFrame();

			uint8_t** get();

			int GetSize();

		private:
			struct Impl;
			UP<Impl> _impl;
		};
	}
}

#endif // AVAudioFrame_h__
