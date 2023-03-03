#ifndef AVFUN_AVFBUFFER_H
#define AVFUN_AVFBUFFER_H

#include <mutex>
#include <condition_variable>

#include "ffmpeg_config.h"
#include "LogUtil.h"

namespace avf {

    struct PacketNode {
        AVPacket *packet{nullptr};
        PacketNode *next{nullptr};
        int serial{0};
    };

    template<size_t _Size>
    struct PacketQueue {
        PacketNode *first{nullptr}, *last{nullptr};
        int nb_packets{0};
        int size{0};
        int serial{0};

        std::mutex mtx;
        std::condition_variable cv_rb;
        std::condition_variable cv_wb;


        void push(AVPacket *packet) {
            std::unique_lock<std::mutex> lock(mtx);

            if (size > _Size) {
                cv_wb.wait(lock);
            }

            PacketNode *node = (PacketNode *) malloc(sizeof(PacketNode));
            node->packet = av_packet_alloc();
            node->serial = serial;
            node->next = nullptr;
            av_packet_move_ref(node->packet, packet);

            if (!first) {
                first = node;
            } else {
                last->next = node;
            }
            last = node;

            nb_packets++;

            size += sizeof(PacketNode) + node->packet->size;

            //LOG_INFO("push pkg: [%d] %d -- %d | %lld", node->packet->stream_index, nb_packets, size, node->packet->pts);

            cv_rb.notify_one();

        }

        void peek(AVPacket *packet,int& serial) {
            std::unique_lock<std::mutex> lock(mtx);

            while (nb_packets == 0 || size == 0) {
                cv_rb.wait(lock);
            }

            PacketNode *node = first;
            serial = node->serial;
            first = node->next;
            if (!first) {
                last = nullptr;
            }

            av_packet_move_ref(packet, node->packet);

            av_packet_free(&node->packet);
            free(node);

            nb_packets--;
            size -= sizeof(PacketNode) + packet->size;

            //LOG_INFO("peek pkg: [%d] %d -- %d | %lld", packet->stream_index, nb_packets, size, packet->pts);

            cv_wb.notify_one();
        }

        void clear() {
            std::unique_lock<std::mutex> lock(mtx);

            if (size == 0){
                return;
            }

            while (last){
                PacketNode *node = first;

                first = node->next;
                if (!first) {
                    last = nullptr;
                }

                nb_packets--;
                size -= sizeof(PacketNode) + node->packet->size;

                av_packet_free(&node->packet);
                free(node);
                node = nullptr;
            }

            cv_wb.notify_one();
        }

    };

    struct Frame {
        AVFrame *frame{nullptr};
        double pts{0};
        double duration{0};
        int serial{0};
    };

    struct PicFrame : public Frame {
        int width{0};
        int height{0};
        int pix_fmt;
    };

    struct AudFrame : public Frame{
        int sample_rate{0};
        int channels;
        int channel_layout;
        int samp_fmt;
        int nb_samples{0};
    };

    template<typename T, size_t _Size, typename = std::enable_if_t<std::is_base_of_v<Frame, T>>>
    struct AVFFrameQueue {

        T queue[_Size];
        int rindex{0};
        int rindex_shown{0};
        int windex{0};
        int size{0};

        std::condition_variable cv_rb;
        std::condition_variable cv_wb;
        std::mutex mtx;

        AVFFrameQueue() {

            for (int i = 0; i < _Size; ++i) {
                queue[i].frame = av_frame_alloc();
            }
        }

        ~AVFFrameQueue() {
            for (int i = 0; i < _Size; ++i) {
                av_frame_free(&queue[i].frame);
            }
        }

        void push(AVFrame *frame, double pts) {
            {
                std::unique_lock<std::mutex> lock(mtx);

                if (size == _Size) {
                    cv_wb.wait(lock);
                }
            }

            auto f = &queue[windex];
            f->pts = pts;
            f->width = frame->width;
            f->height = frame->height;
            f->format = frame->format;

            av_frame_move_ref(f->frame, frame);
            LOG_INFO("push frame:[%dx%d]  %d -- %lf", f->width, f->height, size, f->pts);

            windex++;
            if (windex == _Size) {
                windex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size++;
            }

            cv_rb.notify_one();
        }

        void peek(AVFrame *frame) {

            {
                std::unique_lock<std::mutex> lock(mtx);

                if (size - rindex_shown == 0) {
                    cv_rb.wait(lock);
                }
            }

            //LOG_INFO("peek frame:[%dx%d] -- %lf", queue[rindex].width, queue[rindex].height, queue[rindex].pts);

            av_frame_move_ref(frame, queue[rindex].frame);
            av_frame_unref(queue[rindex].frame);


            rindex++;
            if (rindex == _Size) {
                rindex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size--;
            }

            cv_wb.notify_one();
        }

        T *writable() {
            std::unique_lock<std::mutex> lock(mtx);

            if (size == _Size) {
                cv_wb.wait(lock);
            }

            return &queue[windex];
        }

        void push() {
            windex++;
            if (windex == _Size) {
                windex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size++;
            }

            cv_rb.notify_one();
        }

        T *peek_last() {
            return &queue[rindex];
        }

        T *peek_cur() {
            return &queue[(rindex + rindex_shown) % _Size];
        }

        T *peek_next() {
            return &queue[(rindex + rindex_shown + 1) % _Size];
        }

        int nb_remaining() {
            return size - rindex_shown;
        }

        void next() {
            // 保留一帧
            if (!rindex_shown) {
                rindex_shown = 1;
                return;
            }

            av_frame_unref(queue[rindex].frame);

            rindex++;
            if (rindex == _Size) {
                rindex = 0;
            }

            {
                std::unique_lock<std::mutex> lock(mtx);
                size--;
            }

            cv_wb.notify_one();

        }

    };

}


#endif
