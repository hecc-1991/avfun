#ifndef AVVideoStream_h__
#define AVVideoStream_h__

#include "AVStream.h"

namespace avfun
{
	namespace codec {

		class AVVideoStream : public AVStream
		{

		public:

			static SP<AVVideoStream> Make();

		};



	}
}


#endif // AVVideoStream_h__
