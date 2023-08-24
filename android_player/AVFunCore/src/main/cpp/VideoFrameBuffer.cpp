#include "VideoFrameBuffer.h"
#include "LogUtil.h"

VideoFrameBuffer::VideoFrameBuffer(int width, int height, float unit, int count) {
    if (buffer == nullptr) {
        capactiy = width * height * unit * count;
        unitSize = width * height * unit;
        buffer = (char *) malloc(width * height * unit * count);
    }
}

VideoFrameBuffer::~VideoFrameBuffer() {
    if (buffer) {
        free(buffer);
    }
}

char *VideoFrameBuffer::writable(uint64_t frameSize) {
    std::unique_lock <std::mutex> lock(mtx);

    int available = capactiy - size;
    if (available < frameSize) {
        cvWrite.wait(lock);
    }

    return buffer + writeIndex;
}


void VideoFrameBuffer::push(uint64_t frameSize) {
    writeIndex += frameSize;
    writeIndex = writeIndex % capactiy;

    {
        std::unique_lock <std::mutex> lock(mtx);
        size += frameSize;
        size = size % capactiy;
    }
}

int64_t VideoFrameBuffer::readable() {
    std::unique_lock <std::mutex> lock(mtx);
    return size - remain * unitSize;
}

char *VideoFrameBuffer::peekLast() {
    return buffer + readIndex;
}

char *VideoFrameBuffer::peek() {
    int offset = readIndex + remain * unitSize;
    offset %= capactiy;
    return buffer + offset;
}

char *VideoFrameBuffer::peekNext() {
    int offset = readIndex + (remain + 1) * unitSize;
    offset %= capactiy;
    return buffer + offset;
}

void VideoFrameBuffer::pop() {
    // 保留一帧
    if (!remain) {
        remain = 1;
        return;
    }

    readIndex += unitSize;
    readIndex %= capactiy;

    {
        std::unique_lock <std::mutex> lock(mtx);
        size -= unitSize;
    }

    cvWrite.notify_one();
}