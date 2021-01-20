#include "AVVideoReader.h"
#include "AVVideoFrame.h"

#include "ffmpeg_config.h"

#include "LogUtil.h"

namespace avfun
{
	namespace codec {

		enum class ReplyFrameStat : uint32_t
		{
			ReceiveSucess	= 0, // 成功接收一帧数据
			SendAgain		= 1, // 不足一帧，需要继续发包
			FrameEOF		= 2, // 结束帧
			ERROR			= 3, // 解码异常

		};

		class AVVideoReaderInner : public AVVideoReader
		{

		public:
			AVVideoReaderInner(std::string_view filename);
			~AVVideoReaderInner();

			virtual void SetupDecoder() override;
			virtual SP<AVVideoFrame> ReadNextFrame() override;

		private:
			void open(std::string_view filename);
			int open_codec_context(int* stream_idx,AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type);
			ReplyFrameStat decode_packet(AVCodecContext* dec, const AVPacket* pkt);
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
			AVPacket pkt;

			int video_frame_count{ 0 };
		};


		AVVideoReaderInner::AVVideoReaderInner(std::string_view filename){
			open(filename);
		}

		void AVVideoReaderInner::open(std::string_view filename) {
			/* open input file, and allocate format context */
			if (avformat_open_input(&fmt_ctx, filename.data(), NULL, NULL) < 0) {
				LOG_ERROR("Could not open source file %s", filename.data());
				exit(1);
			}

			/* retrieve stream information */
			if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
				LOG_ERROR("Could not find stream information");
				exit(1);
			}

			/* dump input information to stderr */
			av_dump_format(fmt_ctx, 0, filename.data(), 0);
		}

		int AVVideoReaderInner::open_codec_context(int* stream_idx,
			AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
		{
			int ret, stream_index;
			AVStream* st;
			AVCodec* dec = NULL;
			AVDictionary* opts = NULL;

			ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
			if (ret < 0) {
				LOG_ERROR("Could not find %s stream",av_get_media_type_string(type));
				return ret;
			}
			else {
				stream_index = ret;
				st = fmt_ctx->streams[stream_index];

				/* find decoder for the stream */
				dec = avcodec_find_decoder(st->codecpar->codec_id);
				if (!dec) {
					LOG_ERROR("Failed to find %s codec",av_get_media_type_string(type));
					return AVERROR(EINVAL);
				}

				/* Allocate a codec context for the decoder */
				*dec_ctx = avcodec_alloc_context3(dec);
				if (!*dec_ctx) {
					LOG_ERROR("Failed to allocate the %s codec context",av_get_media_type_string(type));
					return AVERROR(ENOMEM);
				}

				/* Copy codec parameters from input stream to output codec context */
				if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
					LOG_ERROR("Failed to copy %s codec parameters to decoder context",av_get_media_type_string(type));
					return ret;
				}

				/* Init the decoders */
				if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
					LOG_ERROR("Failed to open %s codec",av_get_media_type_string(type));
					return ret;
				}
				*stream_idx = stream_index;
			}

			return 0;
		}

		void AVVideoReaderInner::SetupDecoder() {
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

			/* initialize packet, set data to NULL, let the demuxer fill it */
			av_init_packet(&pkt);
			pkt.data = NULL;
			pkt.size = 0;

		}

		ReplyFrameStat AVVideoReaderInner::decode_packet(AVCodecContext* dec, const AVPacket* pkt)
		{
			int ret = 0;

			// submit the packet to the decoder
			ret = avcodec_send_packet(dec, pkt);
			if (ret < 0) {
				LOG_ERROR("Error submitting a packet for decoding (%d)", ret);
				return ReplyFrameStat::ERROR;
			}

			// get all the available frames from the decoder
			while (ret >= 0) {
				ret = avcodec_receive_frame(dec, frame);

				if (ret < 0) {
					// those two return values are special and mean there is no output
					// frame available, but there were no errors during decoding
					if (ret == AVERROR_EOF)
						return ReplyFrameStat::FrameEOF;

					if (ret == AVERROR(EAGAIN))
						return ReplyFrameStat::SendAgain;


					LOG_ERROR("Error during decoding (%d)", ret);
					return ReplyFrameStat::ERROR;
				}

				LOG_INFO("video_frame n:%d coded_n:%d",video_frame_count++, frame->coded_picture_number);

				av_image_copy(video_dst_data, video_dst_linesize,
					(const uint8_t**)(frame->data), frame->linesize,
					pix_fmt, width, height);

				av_frame_unref(frame);

				if (ret == 0)
					break;
			}

			return ReplyFrameStat::ReceiveSucess;
		}

		SP<AVVideoFrame> AVVideoReaderInner::ReadNextFrame() {
			auto ret = ReplyFrameStat::ReceiveSucess;
			/* read frames from the file */
			do 
			{
				auto res = av_read_frame(fmt_ctx, &pkt);
				if (pkt.stream_index == video_stream_idx)
					ret = decode_packet(video_dec_ctx, &pkt);

				if (ret == ReplyFrameStat::ERROR)
					break;

				if (ret == ReplyFrameStat::FrameEOF)
					break;

				if (ret == ReplyFrameStat::ReceiveSucess) {
					auto vframe = make_sp<AVVideoFrame>(width,height);
					vframe->Convert(video_dst_data, video_dst_linesize, (VFrameFmt)pix_fmt);
					return vframe;
				}

			} while (ret == ReplyFrameStat::SendAgain);

			return nullptr;
		}


		AVVideoReaderInner::~AVVideoReaderInner() {

		}

		UP<AVVideoReader> AVVideoReader::Make(std::string_view filename) {
			auto p = make_up<AVVideoReaderInner>(filename);
			return p;
		}

	}
}

