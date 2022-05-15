
#ifndef AVFUN_AVFAUDIOREADER_H
#define AVFUN_AVFAUDIOREADER_H

#include "AVCommon.h"

namespace avf
{
	namespace codec
	{
        struct AudioParam {
            int sample_rate;
            int channels;
            int channel_layout;
            int sample_fmt;
            int nb_samples;
        };

        struct AudioFrame
        {
            uint8_t** data;
            uint8_t* buf;
            unsigned int buf_size;
            int nb_samples;
            double pts;
        };

		class AudioReader
		{
		public:

			static UP<AudioReader> Make(std::string_view filename);

			virtual void SetupDecoder() = 0;
            virtual void ColseDecoder() = 0;
            virtual AudioParam FetchInfo() = 0;
            virtual SP<AudioFrame> ReadNextFrame() = 0;
            virtual SP<AudioFrame> ReadFrameAt(int64_t pos) = 0;

        };

	}
}

#endif // AVAudioReader_h__
