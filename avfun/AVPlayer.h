#ifndef AVPlayer_h__
#define AVPlayer_h__

#include <string>
#include "AVCommon.h"

namespace avfun
{
	class AVPlayer
	{
	public:
		AVPlayer();
		~AVPlayer();
	public:
		void OpenVideo(std::string_view filename);

		void Start();

		void Pause();

		void Seek(int64_t time_ms);

	private:
		struct Impl;
		UP<Impl> _impl;


		
	};
}

#endif // AVPlayer_h__
