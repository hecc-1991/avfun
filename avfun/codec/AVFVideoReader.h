#ifndef AVFUN_AVFVIDEOREADER_H
#define AVFUN_AVFVIDEOREADER_H

#include "AVCommon.h"
#include "AVFBuffer.h"

namespace avf
{
	namespace codec {

		class AVVideoFrame;

		struct VideoParam {
			int width;
			int height;
			int pix_fmt;
		};

		class VideoReader
		{
		public:

			static UP<VideoReader> Make(std::string_view filename);
			static UP<VideoReader> Make(int fd);

			virtual void SetupDecoder() = 0;
            virtual void ColseDecoder() = 0;
			virtual AVFSizei GetSize() = 0;
			virtual VideoParam GetParam() = 0;
            virtual double GetDuration() = 0;
            virtual SP<AVVideoFrame>  ReadNextFrame() = 0;
			virtual int NbRemaining() = 0;
			virtual PicFrame* PeekLast() = 0;
			virtual PicFrame* PeekCur() = 0;
			virtual PicFrame* PeekNext() = 0;
			virtual void Next() = 0;
			virtual void NextAt(int64_t pos) = 0;
            virtual bool Serial() = 0;

		};

	}
}

#endif
