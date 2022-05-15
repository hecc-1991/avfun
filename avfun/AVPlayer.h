#ifndef AVPlayer_h__
#define AVPlayer_h__

#include <string>
#include "AVCommon.h"

namespace avf
{
    namespace codec{
        class AVVideoFrame;
    }

    enum class PLAYER_STAT : int{
        NONE = 0,
        INIT,
        PLAYING,
        PAUSE,
        STOP,
    };

	class AVPlayer
	{
	public:
		AVPlayer();
		~AVPlayer();
	public:
		void OpenVideo(std::string_view filename);
		void CloseVideo();

		void Play();

		void Pause();

		void Seek(int64_t time_ms);

        AVFSizei GetSize();

        int GetFrame(SP<codec::AVVideoFrame> frame);

        double GetDuration();

        double GetProgress();

    PROPERTY(PLAYER_STAT,Stat);

    private:
		struct Impl;
		UP<Impl> _impl;


    };
}

#endif // AVPlayer_h__
