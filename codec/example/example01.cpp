#include <stdlib.h>
#include "AVVideoStream.h"
#include "AVVideoReader.h"

using namespace avfun::codec;

int main(int argc,char* args[]) {

	constexpr auto video_filename = "C:/hecc/develop/github/avfun/codec/example/resource/video01.mp4";
	auto vr = AVVideoReader::Make(video_filename);

	vr->SetupDecoder();
	vr->ReadNextFrame();

	return 0;
}