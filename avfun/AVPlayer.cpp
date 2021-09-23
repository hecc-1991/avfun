#include "AVPlayer.h"

#include <thread>
#include <mutex>
#include <algorithm>

#include "codec/ffmpeg_config.h"
#include "LogUtil.h"
#include <chrono>
using namespace std::chrono_literals;

namespace avfun
{
	struct PacketList
	{
		AVPacket pkt;
		PacketList* next;
	};

	struct PacketQueue
	{
		PacketList* first_pkt, * last_pkt;
		int nb_packets;
		int size;

		int abort_request;
		std::mutex mtx;
		std::condition_variable cond;

		void packet_queue_put(PacketList* pkt) {
			

		}
	};

	struct Frame
	{
		AVFrame* frame;

		int width;
		int height;
		int format;
	};

#define FRAME_QUEUE_SIZE 20
	struct FrameQueue
	{
		Frame queue[FRAME_QUEUE_SIZE];
		int rindex; // 读索引
		int windex; // 写索引
		int size;	// 帧数
		int max_size;
		int keep_last;
		int rindex_shown{ 0 };
		std::mutex mtx;
		std::condition_variable cond;
		PacketQueue *pktq;

		int frame_queue_init(PacketQueue* pktq, int max_size,int keep_last) {

			this->pktq = pktq;
			this->max_size = std::min(max_size, FRAME_QUEUE_SIZE);
			this->keep_last = !!keep_last;
			for (size_t i = 0; i < max_size; i++)
			{
				if (queue[i].frame = av_frame_alloc()) {
					return AVERROR(ENOMEM);
				}
			}

			return 0;
		}

		void frame_queue_unref_item(Frame* vp) {
			av_frame_unref(vp->frame);
		}

		void frame_queue_destroy() {
			for (size_t i = 0; i < max_size; i++)
			{
				Frame* vp = &queue[i];
				frame_queue_unref_item(vp);
				av_frame_free(&vp->frame);
			}
		}

		Frame* frame_queue_peek_writable() {

			std::unique_lock<std::mutex> lock(mtx);
			if (size >= max_size && !pktq->abort_request)
			{
				cond.wait(lock);
			}

			if (pktq->abort_request)
				return NULL;

			return &queue[windex];
		}

		void frame_queue_push() {
			if (++windex > max_size)
				windex = 0;

			std::unique_lock<std::mutex> lock(mtx);
			size++;
			cond.notify_one();
		}

		int queue_picture(AVFrame* frame){
			Frame *vp;
			if (vp = frame_queue_peek_writable())
				return -1;

			vp->width = frame->width;
			vp->height = frame->height;
			vp->format = frame->format;

			av_frame_move_ref(vp->frame, frame);
			frame_queue_push();
			return 0;
		}

		Frame* frame_queue_peek_readable() {
			std::unique_lock<std::mutex> lock(mtx);
			if (size - rindex_shown <= 0 && !pktq->abort_request)
				cond.wait(lock);

			if (pktq->abort_request)
				return nullptr;

			return &queue[(rindex + rindex_shown) % max_size];
		}

		Frame* frame_queue_peek() {
			return &queue[(rindex + rindex_shown) % max_size];
		}

		Frame* frame_queue_peek_last() {
			return &queue[rindex + rindex_shown];
		}

		Frame* frame_queue_peek_next() {
			return &queue[(rindex + rindex_shown + 1) % max_size];

		}

		void frame_queue_next() {
			if (keep_last && !rindex_shown) {
				rindex_shown = 1;
				return;
			}
			
			frame_queue_unref_item(&queue[rindex]);

			std::unique_lock<std::mutex> lock(mtx);
			if(++rindex == max_size)
				rindex = 0;
			size--;
			cond.notify_one();
		}
	};

	struct VideoState
	{
		FrameQueue pictq; // 视频frame队列
		FrameQueue sampq; // 音频frame队列
	};

	struct AVPlayer::Impl
	{
		AVFormatContext* fmt_ctx{ nullptr };
		AVCodecContext* video_dec_ctx{ nullptr };
		AVCodecContext* audio_dec_ctx{ nullptr };
		int video_stream_idx;
		int audio_stream_idx;
		AVStream* video_stream{ nullptr };
		AVStream* audio_stream{ nullptr };

		int width;
		int height;
		AVPixelFormat pix_fmt;

		uint8_t* video_dst_data[4] = { NULL };
		int      video_dst_linesize[4];

		AVFrame* frame{ nullptr };
		AVPacket pkt;

		int video_frame_count{ 0 };


		std::thread thv;
		std::thread tha;

		VideoState videoState;

		//////////////////////////////////////////////////////////////////////////

		void deocdeVideo() {
			std::this_thread::sleep_for(3000ms);
			LOG_INFO("deocdeVideo");
		}

		void deocdeAudio() {
			LOG_INFO("deocdeAudio");
		}

		int open_codec_context(int* stream_idx,
			AVCodecContext** dec_ctx, AVFormatContext* fmt_ctx, enum AVMediaType type)
		{
			int ret, stream_index;
			AVStream* st;
			AVCodec* dec = NULL;
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
				dec = avcodec_find_decoder(st->codecpar->codec_id);
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

		void demux(std::string_view filename) {

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
			}
			else {
				thv = std::thread(&AVPlayer::Impl::deocdeVideo, this);
			}


			if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
				audio_stream = fmt_ctx->streams[audio_stream_idx];
			}

			
			if (!audio_stream)
			{
				LOG_ERROR("Could not find audio stream in the input, aborting");
			}
			else {
				tha = std::thread(&AVPlayer::Impl::deocdeAudio, this);
			}

			thv.join();
			tha.join();

		}

		void openVideo(std::string_view filename) {

			//解复用线程 
			std::thread th(&AVPlayer::Impl::demux,this,filename);
			th.join();
		}
	};



	AVPlayer::AVPlayer():_impl(make_up<Impl>()){}

	AVPlayer::~AVPlayer(){}

	void AVPlayer::OpenVideo(std::string_view filename)
	{
		_impl->openVideo(filename);
	}

	void AVPlayer::Start() {

	}

	void AVPlayer::Pause() {

	}

	void AVPlayer::Seek(int64_t time_ms) {

	}
}