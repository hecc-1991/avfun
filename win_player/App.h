#ifndef App_h__
#define App_h__
#include <AVCommon.h>

using namespace avf;

class App
{
public:
	
	static UP<App> Make();

	virtual void Run() = 0;
};

#endif // App_h__
