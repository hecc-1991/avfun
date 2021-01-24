#ifndef LogUtil_h__
#define LogUtil_h__

#include <assert.h>

namespace avfun
{

#ifdef _WIN32

#define LOG_INFO(format,...) fprintf(stdout, "[info]#avfun# %s:%d(%s)::"#format"\n",__FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_DEBUG(format,...) fprintf(stdout, "[debug]#avfun# %s:%d(%s)::"#format"\n",__FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_WARNING(format,...) fprintf(stdout, "[warning]#avfun# %s:%d(%s)>>::"#format"\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_ERROR(format,...) fprintf(stdout, "[error]#avfun# %s:%d(%s)::"#format"\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)

#elif __ANDROID__



#else

#endif // _WIN32

#define AV_Assert(expression)                                                              \
            if(!expression) {                                                               \
                LOG_ERROR("assert!");                                                       \
                assert(expression);                                                         \
            }                                                                               
}

#endif // LogUtil_h__
