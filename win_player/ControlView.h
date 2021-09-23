#ifndef ControlView_h__
#define ControlView_h__

#include <AVCommon.h>

using namespace avfun;

class ControlView
{
public:

	ControlView();
	~ControlView();

	void Draw();

private:
	struct Impl;
	UP<Impl> _impl;
};

#endif // ControlView_h__
