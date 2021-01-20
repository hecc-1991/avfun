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
			AVAudioFrame();
			~AVAudioFrame();


			uint8_t*** getAddr();
			uint8_t** get();

			void SetLinesize(int size);
			int GetLinesize();

		private:
			uint8_t** data{ NULL };
			int linesize{ 0 };
		};
	}
}

#endif // AVAudioFrame_h__
