#ifndef App_h__
#define App_h__
#include <AVCommon.h>

using namespace avfun;

class App
{
public:
	
	static UP<App> Make();

	virtual void Run() = 0;
};

#endif // App_h__
