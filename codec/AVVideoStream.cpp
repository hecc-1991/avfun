#include "AVVideoStream.h"
#include "AVVideoReader.h"

namespace avfun
{
	namespace codec {

		class AVVideoStreamInner : public AVVideoStream
		{
		public:
			AVVideoStreamInner() = default;
			~AVVideoStreamInner() = default;

			virtual void Read(std::string_view filename) override;


		private:
			UP<AVVideoReader> reader;
		};


		void AVVideoStreamInner::Read(std::string_view filename) {
			reader = AVVideoReader::Make(filename);
		}

		SP<AVVideoStream> AVVideoStream::Make() {
			auto p = std::make_shared<AVVideoStreamInner>();
			return p;
		}

	}
}

