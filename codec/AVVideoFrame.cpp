#include "AVVideoFrame.h"
#include <libyuv.h>
#include "LogUtil.h"

namespace avfun
{
	namespace codec
	{

#define FRAME_BGRA_CHANNELS 4

		AVVideoFrame::AVVideoFrame(int width, int height):
			width(width),height(height) {
			pData = std::make_unique<uint8_t[]>(width * height * FRAME_BGRA_CHANNELS);
		}

		void AVVideoFrame::Convert(uint8_t** data, int* linesize, VFrameFmt fmt) {
			if (fmt == VFrameFmt::YUV420P)
			{
				libyuv::I420ToARGB(data[0], linesize[0], data[1], linesize[1], data[2], linesize[2], pData.get(),width * FRAME_BGRA_CHANNELS,width,height);
			}
			else if (fmt == VFrameFmt::NV12)
			{
				// todo
			}
			else if (fmt == VFrameFmt::NV21)
			{
				// todo
			}
			else
			{
				LOG_ERROR("invalid format for %d", fmt);
			}
		}

		int AVVideoFrame::GetWidth() {
			return width;
		}

		int AVVideoFrame::GetHeight() {
			return height;
		}

		uint8_t* AVVideoFrame::get() {
			return pData.get();
		}


	}
}