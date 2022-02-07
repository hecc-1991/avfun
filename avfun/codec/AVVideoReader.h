#ifndef AVVideoReader_h__
#define AVVideoReader_h__

#include "AVCommon.h"

namespace avfun
{
	namespace codec {

		class AVVideoFrame;

		class AVVideoReader
		{
		public:

			static UP<AVVideoReader> Make(std::string_view filename);

			virtual void SetupDecoder() = 0;
			virtual SP<AVVideoFrame>  ReadNextFrame() = 0;

            virtual AVFSizei GetSize() = 0;

		private:

		};

	}
}

#endif // AVVideoReader_h__
