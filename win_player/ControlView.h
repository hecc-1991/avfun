#ifndef ControlView_h__
#define ControlView_h__

#include "AVCommon.h"
#include "AVPlayer.h"

using namespace avf;

class ControlView
{
public:

	ControlView();
	~ControlView();

    void SetPlayer(SP<AVPlayer> player);

    void Draw();

private:
	struct Impl;
	UP<Impl> _impl;
};

#endif // ControlView_h__
