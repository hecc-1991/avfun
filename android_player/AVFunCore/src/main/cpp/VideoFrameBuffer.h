#ifndef AVFUNPLAYER_VIDEOFRAMEBUFFER_H
#define AVFUNPLAYER_VIDEOFRAMEBUFFER_H

#include <mutex>

class VideoFrameBuffer {
public:
    VideoFrameBuffer(int width, int height, float unit, int count);

    ~VideoFrameBuffer();

    char *writable(uint64_t frameSize);

    void push(uint64_t frameSize);

    int64_t readable();

    char *peekLast();

    char *peek();

    char *peekNext();

    void pop();

private:

    char *buffer{nullptr};
    uint64_t capactiy{0};
    uint64_t size{0};
    uint64_t unitSize{0};

    uint64_t writeIndex{0};
    uint64_t readIndex{0};
    uint64_t remain{0};

    std::mutex mtx;
    std::condition_variable cvWrite;
    std::condition_variable cvRead;


};


#endif //AVFUNPLAYER_VIDEOFRAMEBUFFER_H
