#include <stdlib.h>
#include <codec/AVVideoReader.h>
#include <codec/AVVideoFrame.h>
#include <opencv2/opencv.hpp>

using namespace avfun::codec;
using namespace cv;

int main(int argc,char* args[]) {

	constexpr auto video_filename = "C:/hecc/develop/github/avfun/avfun/example/resource/video01.mp4";
	auto vr = AVVideoReader::Make(video_filename);

	namedWindow("video");

	vr->SetupDecoder();
	while (true) {
		auto vframe = vr->ReadNextFrame();
		if (vframe == nullptr)break;
		Mat frame(vframe->GetHeight(),vframe->GetWidth(),CV_8UC4,vframe->get());
		imshow("video", frame);
		waitKey(10);
	}


	return 0;
}