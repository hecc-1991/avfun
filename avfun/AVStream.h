#ifndef AVStream_h__
#define AVStream_h__

#include "AVCommon.h"

namespace avfun
{
	namespace codec {
		class AVStream
		{
		public:
			AVStream();
			~AVStream();

			virtual void Read(std::string_view filename) = 0;

		private:

		};


	}
}
#endif // AVStream_h__
