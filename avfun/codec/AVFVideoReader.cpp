#include "AVFVideoReader.h"

#include <thread>

#include "AVFBuffer.h"
#include "AVVideoFrame.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"
#include "AVFReader.h"


namespace avf
{
	namespace codec {

#define VIDEO_PKT_SIZE (4 * 1024 * 1024)

		class VideoReaderInner : public VideoReader, Reader
		{

		public:
			VideoReaderInner(std::string_view filename);
			~VideoReaderInner();

			virtual void SetupDecoder() override;
            virtual void ColseDecoder() override;
            virtual SP<AVVideoFrame> ReadNextFrame() override;
            virtual AVFSizei GetSize() override;

		private:
//			void open(std::string_view filename);
            void read_packet();
            void deocde_frame();
            int decode_packet();

        private:
			AVFormatContext* fmt_ctx{ nullptr };
			AVCodecContext* video_dec_ctx{ nullptr };
			int video_stream_idx;
			AVStream* video_stream{ nullptr };

			int width;
			int height;
			AVPixelFormat pix_fmt;

			uint8_t* video_dst_data[4] = { NULL };
			int      video_dst_linesize[4];

			AVFrame* frame{ nullptr };
			AVPacket* pkt;
            AVPacket* pkt2;

			int video_frame_count{ 0 };

            std::thread th_pkt;
            std::thread th_de_frame;

            bool th_pkt_abort{false};
            bool th_de_frame_abort{false};


            PacketQueue<VIDEO_PKT_SIZE> pkt_queue;
            AVFFrameQueue<PicFrame,3> frame_queue;

        };

/*		void VideoReaderInner::open(std::string_view filename) {
			*//* open input file, and allocate format context *//*
            int ret;
            ret = avformat_open_input(&fmt_ctx, filename.data(), NULL, NULL);
			if (ret < 0) {
				LOG_ERROR("Could not open source file %s", filename.data());
			}
            AV_Assert(!ret);

			*//* retrieve stream information *//*
            ret = avformat_find_stream_info(fmt_ctx, NULL);
			if (ret < 0) {
				LOG_ERROR("Could not find stream information");
			}
            AV_Assert(!ret);

			*//* dump input information to stderr *//*
			av_dump_format(fmt_ctx, 0, filename.data(), 0);
		}*/

		/*int VideoReaderInner::open_codec_context(int* stream_idx,
			AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
		{
			int ret, stream_index;
			AVStream* st;
			AVDictionary* opts = NULL;

			ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
			if (ret < 0) {
				LOG_ERROR("Could not find %s stream",av_get_media_type_string(type));
				return ret;
			}
			else {
				stream_index = ret;
				st = fmt_ctx->streams[stream_index];

				*//* find decoder for the stream *//*
				auto dec = avcodec_find_decoder(st->codecpar->codec_id);
				if (!dec) {
					LOG_ERROR("Failed to find %s codec",av_get_media_type_string(type));
					return AVERROR(EINVAL);
				}

				*//* Allocate a codec context for the decoder *//*
				*dec_ctx = avcodec_alloc_context3(dec);
				if (!*dec_ctx) {
					LOG_ERROR("Failed to allocate the %s codec context",av_get_media_type_string(type));
					return AVERROR(ENOMEM);
				}

				*//* Copy codec parameters from input stream to output codec context *//*
				if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
					LOG_ERROR("Failed to copy %s codec parameters to decoder context",av_get_media_type_string(type));
					return ret;
				}

				*//* Init the decoders *//*
				if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
					LOG_ERROR("Failed to open %s codec",av_get_media_type_string(type));
					return ret;
				}
				*stream_idx = stream_index;
			}

			return 0;
		}*/

        void VideoReaderInner::read_packet() {
            LOG_INFO("## video read_packet");

            while (true) {

                if(th_pkt_abort)
                    break;

                auto res = av_read_frame(fmt_ctx, pkt);
                if (res != 0) {
                    LOG_ERROR("failed to av_read_frame: %lld", res);
                    av_packet_free(&pkt);
                    break;
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
                        LOG_ERROR("## eof");
                        return 1;
                    }

                } while (ret != AVERROR(EAGAIN));

                {
                    pkt_queue.peek(pkt2);
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

                if(th_de_frame_abort)
                    break;

                auto ret = decode_packet();

                if (!ret) {

                    auto picf = frame_queue.writable();
                    if (picf){
                        picf->pts = frame->pts * av_q2d(video_stream->time_base);
                        av_frame_move_ref(picf->frame, frame);
                        picf->width = frame->width;
                        picf->height = frame->height;
                        picf->pix_fmt = frame->format;

                        frame_queue.push();
                        av_frame_unref(frame);
                    }
                }
            }
        }

////////////////////////////////////////////////////////////////////////////////

        VideoReaderInner::VideoReaderInner(std::string_view filename){
            open(filename,&fmt_ctx);
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

				int ret = av_image_alloc(video_dst_data, video_dst_linesize,
					width, height, pix_fmt, 1);

				if (ret < 0) {
					LOG_ERROR("Could not allocate raw video buffer");
					return;
				}
			}

			if (!video_stream)
			{
				LOG_ERROR("Could not find video stream in the input, aborting");
				return;
			}

			frame = av_frame_alloc();
			if (!frame) {
				LOG_ERROR("Could not allocate frame");
				return;
			}

            pkt = av_packet_alloc();
            pkt2 = av_packet_alloc();

            th_pkt = std::thread(&VideoReaderInner::read_packet,this);

            th_de_frame = std::thread(&VideoReaderInner::deocde_frame, this);

        }

        void VideoReaderInner::ColseDecoder()
        {
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
            auto f = make_sp<AVVideoFrame>(frame->width,frame->height);
            f->Convert(frame->data,frame->linesize,(VFrameFmt)frame->format);
            av_frame_free(&frame);
			return f;
		}

        AVFSizei VideoReaderInner::GetSize() {
            AVFSizei s;
            s.width = width;
            s.height = height;
            return s;
        }



		UP<VideoReader> VideoReader::Make(std::string_view filename) {
			auto p = make_up<VideoReaderInner>(filename);
			return p;
		}

	}
}

