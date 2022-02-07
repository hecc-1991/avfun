#ifndef AVVideoFrame_h__
#define AVVideoFrame_h__

#include "AVCommon.h"

namespace avfun
{
	namespace codec
	{
		// ?????????????????
		enum class VFrameFmt : int32_t
		{
			YUV420P = 0,	// (I420) YYYYYYYY UU VV range:[16-235]
            YUVJ420P = 12,   //  range: [0-255]
			NV12	= 23,	// YYYYYYYY UVUV
			NV21	= 24,	// YYYYYYYY VUVU
		};


		class AVVideoFrame
		{
			public:
				AVVideoFrame(int width,int height);
				~AVVideoFrame() = default;

				void Convert(uint8_t** data, int* linesize, VFrameFmt fmt);

				int GetWidth();

				int GetHeight();

				uint8_t* get();

                void reset();

        private:
			int width{ 0 };
			int height{ 0 };
			UP<uint8_t[]> pData;

		};
	}
}

#endif // AVVideoFrame_h__
