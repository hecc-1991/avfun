#include "AVFAudioReader.h"

#include <thread>

#include "AVFBuffer.h"
#include "AVFAudioResample.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"
#include "AVFReader.h"

using namespace std::literals;

namespace avf {
    namespace codec {

#define AUDIO_PKT_SIZE (1/4 * 1024 * 1024)

        class AudioReaderInner : public AudioReader, Reader {

        public:
            AudioReaderInner(std::string_view filename);

            ~AudioReaderInner();

            virtual void SetupDecoder() override;

            virtual void ColseDecoder() override;

            virtual AudioParam FetchInfo() override;

            virtual SP<AudioFrame> ReadNextFrame() override;

            virtual SP<AudioFrame> ReadFrameAt(int64_t pos) override;

        private:
            void read_packet();

            void deocde_frame();

            int decode_packet();

        private:
            AVFormatContext *fmt_ctx{nullptr};
            AVCodecContext *audio_dec_ctx{nullptr};
            int audio_stream_idx;
            AVStream *audio_stream{nullptr};

            AVFrame *frame{nullptr};
            AVPacket *pkt;
            AVPacket *pkt2;

            AudioResample resampler;
            SP<AudioFrame> out_frame;

            std::thread th_pkt;
            std::thread th_de_frame;

            bool th_pkt_abort{false};
            bool th_de_frame_abort{false};

            PacketQueue<AUDIO_PKT_SIZE> pkt_queue;
            AVFFrameQueue<AudFrame, 9> frame_queue;

            int serial{0};
            int seek_pos{0};
            int seek_req{0};

            std::mutex mtx;
            std::condition_variable cv_continue_read;
            int eof{0};
        };


        void AudioReaderInner::read_packet() {
            LOG_INFO("## audio read_packet");

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
                        LOG_WARNING("##read audio pkt eof");
                        pkt->stream_index = audio_stream_idx;
                        pkt_queue.push(pkt);
                        eof = 1;
                    }

                    std::unique_lock<std::mutex> lock(mtx);
                    cv_continue_read.wait(lock);
                    continue;
                } else {
                    eof = 0;
                }

                if (pkt->stream_index == audio_stream_idx) {
                    pkt_queue.push(pkt);
                }

                av_packet_unref(pkt);
            }
        }


        int AudioReaderInner::decode_packet() {
            int ret = 0;

            for (;;) {

                do {

                    ret = avcodec_receive_frame(audio_dec_ctx, frame);

                    if (ret == 0) {
                        return 0;
                    }

                    if (ret == AVERROR_EOF) {
                        LOG_WARNING("##read aduio frame eof");
                        avcodec_flush_buffers(audio_dec_ctx);
                        return 1;
                    }

                } while (ret != AVERROR(EAGAIN));

                do {
                    int old_serial = serial;
                    pkt_queue.peek(pkt2, serial);
                    if (old_serial != serial) {
                        avcodec_flush_buffers(audio_dec_ctx);
                    }

                    if (pkt_queue.serial == serial)
                        break;
                    av_packet_unref(pkt2);
                } while (1);

                {
                    ret = avcodec_send_packet(audio_dec_ctx, pkt2);
                    av_packet_unref(pkt2);

                    if (ret < 0) {
                        LOG_ERROR("Error submitting a packet for decoding (%d)", ret);
                        return -1;
                    }
                }
            }
        }


        void AudioReaderInner::deocde_frame() {
            LOG_INFO("## audio deocde_frame");

            while (true) {

                if (th_de_frame_abort)
                    break;

                auto ret = decode_packet();

                if (!ret) {

                    auto audf = frame_queue.writable();
                    if (audf) {
                        audf->pts = frame->pts * av_q2d(audio_stream->time_base);
                        audf->sample_rate = frame->sample_rate;
                        audf->channels = frame->channels;
                        audf->channel_layout = frame->channel_layout;
                        audf->samp_fmt = frame->format;
                        audf->nb_samples = frame->nb_samples;
                        audf->serial = serial;
                        av_frame_move_ref(audf->frame, frame);

                        frame_queue.push();
                        av_frame_unref(frame);
                    }
                }
            }
        }



////////////////////////////////////////////////////////////////////////////////


        AudioReaderInner::AudioReaderInner(std::string_view filename) {
            open(filename, &fmt_ctx);

        }

        AudioReaderInner::~AudioReaderInner() {
            avformat_close_input(&fmt_ctx);
        }

        void AudioReaderInner::SetupDecoder() {
            if (fmt_ctx == NULL)return;

            if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
                audio_stream = fmt_ctx->streams[audio_stream_idx];
            }

            if (!audio_stream) {
                LOG_ERROR("Could not find audio stream in the input, aborting");
                return;
            }

            frame = av_frame_alloc();
            if (!frame) {
                LOG_ERROR("Could not allocate frame");
                return;
            }

            pkt = av_packet_alloc();
            pkt2 = av_packet_alloc();


            resampler.Setup(FetchInfo());
            out_frame = make_sp<AudioFrame>();

            th_pkt = std::thread(&AudioReaderInner::read_packet, this);

            th_de_frame = std::thread(&AudioReaderInner::deocde_frame, this);

        }

        void AudioReaderInner::ColseDecoder() {
            avcodec_free_context(&audio_dec_ctx);
            //avformat_close_input(&fmt_ctx);
            av_frame_free(&frame);
            av_packet_free(&pkt);
            av_packet_free(&pkt2);

            if (out_frame->buf)
                av_freep(out_frame->buf);

            th_pkt_abort = true;
            th_pkt.join();

            th_de_frame_abort = true;
            th_de_frame.join();
        }

        AudioParam AudioReaderInner::FetchInfo() {
            AudioParam info;
            info.sample_rate = audio_stream->codecpar->sample_rate;
            info.channels = audio_stream->codecpar->channels;
            info.channel_layout = audio_stream->codecpar->channel_layout;
            info.sample_fmt = audio_stream->codecpar->format;
            info.nb_samples = audio_stream->codecpar->frame_size;

            return info;
        }


        SP<AudioFrame> AudioReaderInner::ReadNextFrame() {

            if (frame_queue.nb_remaining() == 0){
                return nullptr;
            }

            auto in = make_sp<AudioFrame>();

            AVFrame *f = av_frame_alloc();
            frame_queue.peek(f);

            in->data = f->data;
            in->nb_samples = f->nb_samples;

            resampler.Convert(in, out_frame);

            av_frame_free(&f);

            return out_frame;

        }

        SP<AudioFrame> AudioReaderInner::ReadFrameAt(int64_t pos) {

            seek_pos = pos;
            seek_req = 1;
            cv_continue_read.notify_one();


            while (true) {
                if (frame_queue.nb_remaining() == 0) {
                    continue;
                }

                auto last = frame_queue.peek_last();
                auto cur = frame_queue.peek_cur();
                auto last_pts = last->pts * AVF_TIME_BASE;
                auto cur_pts = cur->pts * AVF_TIME_BASE;

//                LOG_ERROR("last_pts:%lf, cur_pts:%lf, pos:%lld", last_pts, cur_pts, pos);

                if (pos >= last_pts && pos < cur_pts) {

                    return ReadNextFrame();
                }
                frame_queue.next();
            }

        }

        UP<AudioReader> AudioReader::Make(std::string_view filename) {
            auto p = make_up<AudioReaderInner>(filename);
            return p;
        }
    }
}