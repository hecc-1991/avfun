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

        }


        int AudioReaderInner::decode_packet() {
            int ret = 0;

            return ret;
        }


        void AudioReaderInner::deocde_frame() {
            LOG_INFO("## audio deocde_frame");

        }



////////////////////////////////////////////////////////////////////////////////


        AudioReaderInner::AudioReaderInner(std::string_view filename) {

        }

        AudioReaderInner::~AudioReaderInner() {
        }

        void AudioReaderInner::SetupDecoder() {


        }

        void AudioReaderInner::ColseDecoder() {

        }

        AudioParam AudioReaderInner::FetchInfo() {
            AudioParam info;


            return info;
        }


        SP<AudioFrame> AudioReaderInner::ReadNextFrame() {


            return out_frame;

        }

        SP<AudioFrame> AudioReaderInner::ReadFrameAt(int64_t pos) {

            return nullptr;

        }

        UP<AudioReader> AudioReader::Make(std::string_view filename) {
            auto p = make_up<AudioReaderInner>(filename);
            return p;
        }
    }
}