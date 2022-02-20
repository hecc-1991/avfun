#include <stdlib.h>
#include <codec/AVFVideoReader.h>
#include <codec/AVVideoFrame.h>
#include <opencv2/opencv.hpp>

using namespace avf::codec;
using namespace cv;

int main(int argc,char* args[]) {

	constexpr auto video_filename = "/Users/hecc/Documents/hecc/dev/avfun/resources/love.MP4";
	auto vr = VideoReader::Make(video_filename);

	namedWindow("video");

	vr->SetupDecoder();
	while (true) {
		auto vframe = vr->ReadNextFrame();
        if (vframe == nullptr)continue;
		Mat frame(vframe->GetHeight(),vframe->GetWidth(),CV_8UC4,vframe->get());
		imshow("video", frame);
		waitKey(10);
	}


	return 0;
}