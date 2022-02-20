#ifndef AVFUN_AVFVIDEOREADER_H
#define AVFUN_AVFVIDEOREADER_H

#include "AVCommon.h"

namespace avf
{
	namespace codec {

		class AVVideoFrame;

		class VideoReader
		{
		public:

			static UP<VideoReader> Make(std::string_view filename);

			virtual void SetupDecoder() = 0;
            virtual void ColseDecoder() = 0;
            virtual SP<AVVideoFrame>  ReadNextFrame() = 0;

            virtual AVFSizei GetSize() = 0;

		private:

		};

	}
}

#endif // AVVideoReader_h__
