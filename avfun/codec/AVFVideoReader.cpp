#include "AVFVideoReader.h"

#include <thread>

#include "AVVideoFrame.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"
#include "AVFReader.h"


namespace avf {
    namespace codec {

#define VIDEO_PKT_SIZE (4 * 1024 * 1024)

        class VideoReaderInner : public VideoReader, Reader {

        public:
            VideoReaderInner(std::string_view filename);

            ~VideoReaderInner();

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
            void read_packet();

            void deocde_frame();

            int decode_packet();

        private:
            AVFormatContext *fmt_ctx{nullptr};
            AVCodecContext *video_dec_ctx{nullptr};
            int video_stream_idx{-1};
            AVStream *video_stream{nullptr};

            int width;
            int height;
            AVPixelFormat pix_fmt;
            AVRational frame_rate;
            int64_t duration;

            uint8_t *video_dst_data[4] = {NULL};
            int video_dst_linesize[4];

            AVFrame *frame{nullptr};
            AVPacket *pkt;
            AVPacket *pkt2;

            std::thread th_pkt;
            std::thread th_de_frame;

            bool th_pkt_abort{false};
            bool th_de_frame_abort{false};


            PacketQueue<VIDEO_PKT_SIZE> pkt_queue;
            AVFFrameQueue<PicFrame, 3> frame_queue;

            int serial{0};
            int seek_pos{0};
            int seek_req{0};

            std::mutex mtx;
            std::condition_variable cv_continue_read;
            int eof{0};
        };

        void VideoReaderInner::read_packet() {
            LOG_INFO("## video read_packet");

            while (true) {

                if (th_pkt_abort)
                    break;

                if (seek_req) {
                    pkt_queue.clear();
                    auto ts = seek_pos * AVF_TIME_BASE;
                    avformat_seek_file(fmt_ctx, -1, INT64_MIN, ts, INT64_MAX, 0);
                    seek_req = 0;
                    pkt_queue.serial++;
                }

                auto ret = av_read_frame(fmt_ctx, pkt);
                if (ret < 0) {
                    if ((ret == AVERROR_EOF || avio_feof(fmt_ctx->pb)) && !eof) {
                        LOG_WARNING("##read video pkt eof");
                        pkt->stream_index = video_stream_idx;
                        pkt_queue.push(pkt);
                        eof = 1;
                    }

                    std::unique_lock<std::mutex> lock(mtx);
                    cv_continue_read.wait(lock);
                    continue;
                } else {
                    eof = 0;
                }
                if (pkt->stream_index == video_stream_idx) {
                    pkt_queue.push(pkt);
                }

                av_packet_unref(pkt);
            }
        }

        int VideoReaderInner::decode_packet() {

            int ret = 0;

            for (;;) {

                do {

                    ret = avcodec_receive_frame(video_dec_ctx, frame);

                    if (ret == 0) {
                        return 0;
                    }

                    if (ret == AVERROR_EOF) {
                        LOG_WARNING("##read video frame eof");
                        avcodec_flush_buffers(video_dec_ctx);
                        return 1;
                    }

                } while (ret != AVERROR(EAGAIN));

                do {
                    int old_serial = serial;
                    pkt_queue.peek(pkt2, serial);
                    if (old_serial != serial) {
                        avcodec_flush_buffers(video_dec_ctx);
                    }

                    if (pkt_queue.serial == serial)
                        break;
                    av_packet_unref(pkt2);
                } while (1);

                {
                    ret = avcodec_send_packet(video_dec_ctx, pkt2);
                    av_packet_unref(pkt2);

                    if (ret < 0) {
                        LOG_ERROR("Error submitting a packet for decoding (%d)", ret);
                        return -1;
                    }
                }
            }

        }

        void VideoReaderInner::deocde_frame() {
            LOG_INFO("## video deocde_frame");


            while (true) {

                if (th_de_frame_abort)
                    break;

                auto ret = decode_packet();

                if (!ret) {

                    auto picf = frame_queue.writable();
                    if (picf) {
                        picf->pts = frame->pts * av_q2d(video_stream->time_base);
                        picf->duration = (frame_rate.num && frame_rate.den ? av_q2d(
                                (AVRational) {frame_rate.den, frame_rate.num}) : 0);;
                        picf->width = frame->width;
                        picf->height = frame->height;
                        picf->pix_fmt = frame->format;
                        picf->serial = serial;
                        av_frame_move_ref(picf->frame, frame);

                        frame_queue.push();
                        av_frame_unref(frame);
                    }
                }
            }
        }

////////////////////////////////////////////////////////////////////////////////

        VideoReaderInner::VideoReaderInner(std::string_view filename) {
            open(filename, &fmt_ctx);
        }

        VideoReaderInner::~VideoReaderInner() {
            avformat_close_input(&fmt_ctx);
        }

        void VideoReaderInner::SetupDecoder() {
            if (fmt_ctx == NULL)return;

            if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
                video_stream = fmt_ctx->streams[video_stream_idx];

                width = video_dec_ctx->width;
                height = video_dec_ctx->height;
                pix_fmt = video_dec_ctx->pix_fmt;
                duration = fmt_ctx->duration;

                int ret = av_image_alloc(video_dst_data, video_dst_linesize,
                                         width, height, pix_fmt, 1);

                if (ret < 0) {
                    LOG_ERROR("Could not allocate raw video buffer");
                    return;
                }
            }

            if (!video_stream) {
                LOG_ERROR("Could not find video stream in the input, aborting");
                return;
            }
            frame_rate = av_guess_frame_rate(fmt_ctx, video_stream, NULL);

            frame = av_frame_alloc();
            AV_Assert(frame);


            pkt = av_packet_alloc();
            pkt2 = av_packet_alloc();

            th_pkt = std::thread(&VideoReaderInner::read_packet, this);

            th_de_frame = std::thread(&VideoReaderInner::deocde_frame, this);

        }

        void VideoReaderInner::ColseDecoder() {
            avcodec_free_context(&video_dec_ctx);
            //avformat_close_input(&fmt_ctx);
            av_frame_free(&frame);
            av_packet_free(&pkt);
            av_packet_free(&pkt2);
            av_free(video_dst_data[0]);

            th_pkt_abort = true;
            th_pkt.join();

            th_de_frame_abort = true;
            th_de_frame.join();
        }

        SP<AVVideoFrame> VideoReaderInner::ReadNextFrame() {
            AVFrame *frame = av_frame_alloc();
            frame_queue.peek(frame);
            auto f = make_sp<AVVideoFrame>(frame->width, frame->height);
            f->Convert(frame->data, frame->linesize, (VFrameFmt) frame->format);
            av_frame_free(&frame);
            return f;
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
            vp.pix_fmt = (int) pix_fmt;
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
            seek_req = 1;
            cv_continue_read.notify_one();
        }

        bool VideoReaderInner::Serial() {
            auto vp = frame_queue.peek_cur();
            return vp->serial == pkt_queue.serial;
        }


        UP<VideoReader> VideoReader::Make(std::string_view filename) {
            auto p = make_up<VideoReaderInner>(filename);
            return p;
        }

    }
}

