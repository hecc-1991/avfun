#ifndef DisplayView_h__
#define DisplayView_h__

#include "AVCommon.h"
#include "AVPlayer.h"

using namespace avf;

class SurfaceView
{
public:

    SurfaceView();
    ~SurfaceView();

    void SetPlayer(SP<AVPlayer> player);

    void Draw();

private:
    struct Impl;
    UP<Impl> _impl;
};

#endif // DisplayView_h__
