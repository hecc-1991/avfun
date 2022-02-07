#include "AVAudioReader.h"
#include "AVAudioFrame.h"
#include "AVAudioResample.h"
#include "AVAudioBuffer.h"
#include "ffmpeg_config.h"
#include "LogUtil.h"

namespace avfun
{
	namespace codec
	{

		enum class ReplyFrameStat : uint32_t
		{
			ReceiveSucess = 0, // �ɹ�����һ֡����
			SendAgain = 1, // ����һ֡����Ҫ��������
			FrameEOF = 2, // ����֡
			ERROR = 3, // �����쳣

		};

		class AVAudioReaderInner : public AVAudioReader
		{

		public:
			AVAudioReaderInner(std::string_view filename);
			~AVAudioReaderInner();

			virtual void SetupDecoder() override;
			virtual SP<AVAudioFrame> ReadNextFrame() override;

		private:
			void open(std::string_view filename);
			int open_codec_context(int* stream_idx, AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type);
			ReplyFrameStat decode_packet(AVCodecContext* dec, const AVPacket* pkt);
			ReplyFrameStat decode_remain_packet(AVCodecContext* dec);
		private:
			AVFormatContext* fmt_ctx{ nullptr };
			AVCodecContext* audio_dec_ctx{ nullptr };
			int audio_stream_idx;
			AVStream* audio_stream{ nullptr };

			uint8_t* audio_dst_data[4] = { NULL };
			int      audio_dst_linesize[4];

			AVFrame* frame{ nullptr };
			AVPacket pkt;

			int audio_frame_count{ 0 };

			bool eofPacket{false};
			bool eofFrame{false};

			UP<AVAudioResample> audioResample;

			UP<AVAudioBuffer> audioBuffer;
            FILE *ff = NULL;

		};

		AVAudioReaderInner::AVAudioReaderInner(std::string_view filename) {
			open(filename);
            if(!ff)
                ff = fopen("/Users/hecc/Documents/hecc/dev/avfun/resources/nout.pcm","wb");

        }

		void AVAudioReaderInner::open(std::string_view filename) {
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

		int AVAudioReaderInner::open_codec_context(int* stream_idx,
			AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
		{
			int ret, stream_index;
			AVStream* st;
			AVDictionary* opts = NULL;

			ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
			if (ret < 0) {
				LOG_ERROR("Could not find %s stream", av_get_media_type_string(type));
				return ret;
			}
			else {
				stream_index = ret;
				st = fmt_ctx->streams[stream_index];

				/* find decoder for the stream */
				auto dec = avcodec_find_decoder(st->codecpar->codec_id);
				if (!dec) {
					LOG_ERROR("Failed to find %s codec", av_get_media_type_string(type));
					return AVERROR(EINVAL);
				}

				/* Allocate a codec context for the decoder */
				*dec_ctx = avcodec_alloc_context3(dec);
				if (!*dec_ctx) {
					LOG_ERROR("Failed to allocate the %s codec context", av_get_media_type_string(type));
					return AVERROR(ENOMEM);
				}

				/* Copy codec parameters from input stream to output codec context */
				if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
					LOG_ERROR("Failed to copy %s codec parameters to decoder context", av_get_media_type_string(type));
					return ret;
				}

				/* Init the decoders */
				if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
					LOG_ERROR("Failed to open %s codec", av_get_media_type_string(type));
					return ret;
				}
				*stream_idx = stream_index;
			}

			return 0;
		}

		void AVAudioReaderInner::SetupDecoder() {
			if (fmt_ctx == NULL)return;

			if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
				audio_stream = fmt_ctx->streams[audio_stream_idx];
			}

			if (!audio_stream)
			{
				LOG_ERROR("Could not find audio stream in the input, aborting");
				return;
			}

			audioResample = make_up<AVAudioResample>();
			auto data = make_sp<AVAudioStruct>();
			data->sample_rate = audio_stream->codecpar->sample_rate;
			data->channels = audio_stream->codecpar->channels;
			data->sample_fmt = audio_stream->codecpar->format;
			audioResample->Setup(data);

			audioBuffer = make_up<AVAudioBuffer>();

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

		ReplyFrameStat AVAudioReaderInner::decode_packet(AVCodecContext* dec, const AVPacket* pkt)
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

				size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);

				LOG_INFO("audio_frame n:%d nb_samples:%d pts:%lld",
					audio_frame_count++, frame->nb_samples,frame->pts);

				//av_frame_unref(frame);

                if (ret == 0)
                    fwrite(frame->data[0],1,frame->linesize[0],ff);

                if (ret == 0)
					break;
			}

			return ReplyFrameStat::ReceiveSucess;
		}

		ReplyFrameStat AVAudioReaderInner::decode_remain_packet(AVCodecContext* dec) {
			
			int ret = avcodec_receive_frame(dec, frame);

			if (ret < 0) {
				return ReplyFrameStat::FrameEOF;
			}

			size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat)frame->format);

			LOG_INFO("audio_frame n:%d nb_samples:%d pts:%lld",
				audio_frame_count++, frame->nb_samples, frame->pts);

			//av_frame_unref(frame);

			return ReplyFrameStat::ReceiveSucess;
		}

		SP<AVAudioFrame> AVAudioReaderInner::ReadNextFrame() {
			/* read frames from the file */

			do {



				if (audioBuffer->GetBufSize() >= SUPPORT_AUDIO_SAMPLE_NUM ||
					(audioBuffer->GetBufSize() > 0 && eofFrame)) {
					auto aframe = audioBuffer->ReadFrame();
					return aframe;
				}

				if (eofFrame)
					break;

				auto ret = ReplyFrameStat::SendAgain;

				do
				{

					if (!eofPacket)
					{
						auto res = av_read_frame(fmt_ctx, &pkt);
						if (res < 0)
						{
							eofPacket = true;
							LOG_INFO("end of send packet");
						}
					}

					if (pkt.stream_index == audio_stream_idx && !eofPacket)
						ret = decode_packet(audio_dec_ctx, &pkt);

					if (eofPacket)
					{
						auto sss = audioBuffer->GetBufSize();
						ret = decode_remain_packet(audio_dec_ctx);
					}

					if (ret == ReplyFrameStat::ERROR) {
						av_packet_unref(&pkt);
						return nullptr;
					}

					if (ret == ReplyFrameStat::FrameEOF) {
						eofFrame = true;
						LOG_INFO("end of receive frame");
						break;
					}

					if (ret == ReplyFrameStat::ReceiveSucess) {
						auto ast = make_sp<AVAudioStruct>();
						ast->data = frame->extended_data;
						ast->sample_rate = frame->sample_rate;
						ast->channels = frame->channels;
						ast->sample_fmt = frame->format;
						ast->nb_samples = frame->nb_samples;
						int frame_size = audioResample->Resample(ast);
						audioBuffer->AddSamples(audioResample->data(), frame_size);
						av_frame_unref(frame);
						break;
					}

				} while (ret == ReplyFrameStat::SendAgain);

				if (!eofPacket)
					av_packet_unref(&pkt);

			} while (true);

			return nullptr;
		}



		AVAudioReaderInner::~AVAudioReaderInner() {

			avcodec_free_context(&audio_dec_ctx);
			avformat_close_input(&fmt_ctx);
			av_frame_free(&frame);

		}

		UP<AVAudioReader> AVAudioReader::Make(std::string_view filename) {
			auto p = make_up<AVAudioReaderInner>(filename);
			return p;
		}
	}
}