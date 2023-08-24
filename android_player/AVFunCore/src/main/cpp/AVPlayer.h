#ifndef AVFUNPLAYER_AVPLAYER_H
#define AVFUNPLAYER_AVPLAYER_H

#include <string>
#include <thread>
#include <memory>
#include <mutex>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>

#include "VideoFrameBuffer.h"
#include "GLProgram.h"

namespace avf {

class AVPlayer {
public:
    int OpenVideo(std::string filename);

    int Start();

    int Stop();

private:
    void readSample();
    void audioDecode();
    int renderVideo();
    void initGL(int screenWidth,int screenHeight);
private:
    AMediaExtractor *extractor;
    AMediaCodec *videoCodec;
    AMediaCodec *audioCodec;

    std::thread th_videoDecode;
    std::thread th_audioDecode;

    int sawInputEOS;
    int sawOutputEOS;

    int width;
    int height;
    float fmtUnit;

    bool intercepted{false};

    std::shared_ptr<VideoFrameBuffer> videoFrameBuffer;

    UP<render::GLProgram> program;
};
}

#endif
