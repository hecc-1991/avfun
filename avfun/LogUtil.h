#ifndef LogUtil_h__
#define LogUtil_h__

#include <assert.h>

namespace avf
{

#if defined(_WIN32) || defined(__linux__)  || defined(__APPLE__)

#define LOG_INFO(format,...) fprintf(stdout, "[info]#avfun# %s:%d(%s)::"#format"\n",__FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_DEBUG(format,...) fprintf(stdout, "[debug]#avfun# %s:%d(%s)::"#format"\n",__FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_WARNING(format,...) fprintf(stdout, "[warning]#avfun# %s:%d(%s)>>::"#format"\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_ERROR(format,...) fprintf(stderr, "[error]#avfun# %s:%d(%s)::"#format"\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)

#elif __ANDROID__

#else

#endif

#define AV_Assert(expression)                                                              \
            if(!expression) {                                                               \
                assert(expression);                                                         \
            }                                                                               
}

#endif // LogUtil_h__
