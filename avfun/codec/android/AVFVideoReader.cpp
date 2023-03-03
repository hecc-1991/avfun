#include "AVFVideoReader.h"

#include <thread>
#include <stdio.h>
#include <unistd.h>

#include "AVVideoFrame.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"
#include "AVFReader.h"

#include "media/NdkMediaCodec.h"
#include "media/NdkMediaExtractor.h"

#include <android/native_window_jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

namespace avf {
    namespace codec {

#define VIDEO_PKT_SIZE (4 * 1024 * 1024)
#define SEEK_VIDEO_PACKET   1
#define SEEK_VIDEO_FRAME    2

        class VideoReaderInner : public VideoReader, Reader {

        public:
            VideoReaderInner(int fd);

            virtual ~VideoReaderInner();

            virtual void SetupDecoder() override;

            virtual void ColseDecoder() override;

            virtual AVFSizei GetSize() override;

            virtual VideoParam GetParam() override;

            virtual double GetDuration() override;

            virtual SP<AVVideoFrame> ReadNextFrame() override;

            virtual int NbRemaining() override;

            virtual PicFrame *PeekLast() override;

            virtual PicFrame *PeekCur() override;

            virtual PicFrame *PeekNext() override;

            virtual void Next() override;

            virtual void NextAt(int64_t pos) override;

            virtual bool Serial() override;

        private:
            void open(int fd);

            void send_buffer();

            void acquire_buffer();

        private:
            int video_fd{0};
            ANativeWindow *window;
            AMediaExtractor *extractor{nullptr};
            AMediaCodec *codec{nullptr};

            ssize_t bufidx{-1};

            std::thread th_send_buffer;
            std::thread th_acquire_buffer;

            bool sawInputEOS{false};
            bool sawOutputEOS{false};

            bool th_send_buffer_abort{false};
            bool th_acquire_buffer_abort{false};

            int width;
            int height;
            int duration;

            AVFFrameQueue<PicFrame, 3> frame_queue;

            int serial{0};
            int seek_pos{0};
            int seek_req{0};

            std::mutex mtx;
            std::condition_variable cv_send;
            int eof{0};
        };


        void VideoReaderInner::open(int fd) {
            LOG_INFO("@@@ create");

            // convert Java string to UTF-8
//            const char *utf8 = env->GetStringUTFChars(filename, NULL);
//            LOG_INFO("opening %s", utf8);
//
//            off_t outStart, outLen;
//            int fd = AAsset_openFileDescriptor(AAssetManager_open(AAssetManager_fromJava(env, assetMgr), utf8, 0),
//                                               &outStart, &outLen);
//
//            env->ReleaseStringUTFChars(filename, utf8);
//            if (fd < 0) {
//                LOG_ERROR("failed to open file: %s %d (%s)", utf8, fd, strerror(errno));
//                return;
//            }

            off_t outStart, outLen;

            video_fd = fd;

            extractor = AMediaExtractor_new();
            media_status_t err = AMediaExtractor_setDataSourceFd(extractor, video_fd,
                                                                 static_cast<off64_t>(outStart),
                                                                 static_cast<off64_t>(outLen));
            close(video_fd);
            if (err != AMEDIA_OK) {
                LOG_ERROR("setDataSource error: %d", err);
                return;
            }
        }

        void VideoReaderInner::send_buffer() {
            LOG_INFO("## video send_buffer");

            while (true) {
                if (th_send_buffer_abort) {
                    break;
                }

                if (sawInputEOS) {
                    usleep(10 * 1000);
                    continue;
                }

                ssize_t bufidx = AMediaCodec_dequeueInputBuffer(codec, 2000);
                LOG_INFO("input buffer %zd", bufidx);
                if (bufidx >= 0) {
                    size_t bufsize;
                    auto buf = AMediaCodec_getInputBuffer(codec, bufidx, &bufsize);
                    auto sampleSize = AMediaExtractor_readSampleData(extractor, buf, bufsize);
                    if (sampleSize < 0) {
                        sampleSize = 0;
                        sawInputEOS = true;
                        LOG_INFO("EOS");
                    }
                    auto presentationTimeUs = AMediaExtractor_getSampleTime(extractor);

                    AMediaCodec_queueInputBuffer(codec, bufidx, 0, sampleSize, presentationTimeUs,
                                                 sawInputEOS ? AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM
                                                             : 0);
                    AMediaExtractor_advance(extractor);
                }
            }


        }

        void VideoReaderInner::acquire_buffer() {
            LOG_INFO("## video acquire_buffer");

            while (true) {

                if (th_acquire_buffer_abort) {
                    break;
                }

                if (sawOutputEOS || bufidx < 0) {
                    usleep(10 * 1000);
                    continue;
                }

                AMediaCodecBufferInfo info;
                auto status = AMediaCodec_dequeueOutputBuffer(codec, &info, 0);
                if (status >= 0) {
                    if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
                        LOG_INFO("output EOS");
                        sawOutputEOS = true;
                    }
//                    int64_t presentationNano = info.presentationTimeUs * 1000;
                    LOG_INFO("info.presentationTimeUs: %lld", info.presentationTimeUs);
                    size_t out_size;
                    auto buf = AMediaCodec_getOutputBuffer(codec, bufidx, &out_size);

                    auto picf = frame_queue.writable();
                    if (picf) {
                        picf->pts = info.presentationTimeUs / 1000000.0;
                        picf->duration = duration;
                        picf->width = width;
                        picf->height = height;
                        picf->pix_fmt = static_cast<int>(VFrameFmt::YUV420P);

                        if (picf->frame->data == nullptr) {
                            av_image_alloc(picf->frame->data, picf->frame->linesize, width, height,
                                           AV_PIX_FMT_YUV420P, 1);
                        }

                        {
                            memcpy(picf->frame->data[0], buf, width * height);
                            memcpy(picf->frame->data[1], buf, width * height / 4);
                            memcpy(picf->frame->data[2], buf, width * height / 4);
                        }

                        frame_queue.push();
                    }

                    AMediaCodec_releaseOutputBuffer(codec, status, info.size != 0);

                } else if (status == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED) {
                    LOG_INFO("output buffers changed");
                } else if (status == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
                    auto format = AMediaCodec_getOutputFormat(codec);
                    LOG_INFO("format changed to: %s", AMediaFormat_toString(format));
                    AMediaFormat_delete(format);
                } else if (status == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
                    LOG_INFO("no output buffer right now");
                } else {
                    LOG_INFO("unexpected info code: %zd", status);
                }
            }
        }

////////////////////////////////////////////////////////////////////////////////

        VideoReaderInner::VideoReaderInner(int fd) {
            open(fd);
        }

        VideoReaderInner::~VideoReaderInner() {
        }

        void VideoReaderInner::SetupDecoder() {
            int numtracks = AMediaExtractor_getTrackCount(extractor);

            LOG_INFO("input has %d tracks", numtracks);
            for (int i = 0; i < numtracks; i++) {
                AMediaFormat *format = AMediaExtractor_getTrackFormat(extractor, i);
                const char *s = AMediaFormat_toString(format);
                LOG_INFO("track %d format: %s", i, s);
                const char *mime;
                if (!AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime)) {
                    LOG_INFO("no mime type");
                    return;
                } else if (!strncmp(mime, "video/", 6)) {
                    // Omitting most error handling for clarity.
                    // Production code should check for errors.
                    AMediaExtractor_selectTrack(extractor, i);
                    codec = AMediaCodec_createDecoderByType(mime);
                    AMediaCodec_configure(codec, format, window, NULL, 0);
                    AMediaCodec_start(codec);

                    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
                    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
                    AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_DURATION, &duration);

                }
                AMediaFormat_delete(format);
            }

            th_send_buffer = std::thread(&VideoReaderInner::send_buffer, this);
            th_acquire_buffer = std::thread(&VideoReaderInner::acquire_buffer, this);
        }

        void VideoReaderInner::ColseDecoder() {

            th_send_buffer_abort = true;
            th_send_buffer.join();

            th_acquire_buffer_abort = true;
            th_acquire_buffer.join();
        }

        SP<AVVideoFrame> VideoReaderInner::ReadNextFrame() {

            return nullptr;
        }

        AVFSizei VideoReaderInner::GetSize() {
            AVFSizei s;
            s.width = width;
            s.height = height;
            return s;
        }

        VideoParam VideoReaderInner::GetParam() {
            VideoParam vp;
            vp.width = width;
            vp.height = height;
            vp.pix_fmt = static_cast<int>(VFrameFmt::YUV420P);
            return vp;
        }

        double VideoReaderInner::GetDuration() {
            return duration / 1000000.0;
        }

        int VideoReaderInner::NbRemaining() {
            return frame_queue.nb_remaining();
        }

        PicFrame *VideoReaderInner::PeekLast() {
            return frame_queue.peek_last();
        }

        PicFrame *VideoReaderInner::PeekCur() {
            return frame_queue.peek_cur();
        }

        PicFrame *VideoReaderInner::PeekNext() {
            return frame_queue.peek_next();
        }

        void VideoReaderInner::Next() {
            return frame_queue.next();
        }

        void VideoReaderInner::NextAt(int64_t pos) {
            seek_pos = pos;
            seek_req = SEEK_VIDEO_PACKET + SEEK_VIDEO_FRAME;
//            cv_continue_read.notify_one();
        }

        bool VideoReaderInner::Serial() {
            auto vp = frame_queue.peek_cur();
//            return vp->serial == pkt_queue.serial;
            return true;
        }


        UP<VideoReader> VideoReader::Make(std::string_view filename) {
            return nullptr;
        }

        UP<VideoReader> VideoReader::Make(int fd) {
            auto p = make_up<VideoReaderInner>(fd);
            return p;
        }
    }
}

