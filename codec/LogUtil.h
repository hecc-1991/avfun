#ifndef LogUtil_h__
#define LogUtil_h__

namespace avfun
{

#ifdef _WIN32

#define LOG_INFO(format,...) fprintf(stdout, "##avfun## %s:%d(%s)>>[info]::"#format"\n",__FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_DEBUG(format,...) fprintf(stdout, "##avfun## %s:%d(%s)>>[debug]::"#format"\n",__FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_WARNING(format,...) fprintf(stdout, "##avfun## %s:%d(%s)>>[warning]::"#format"\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)
#define LOG_ERROR(format,...) fprintf(stdout, "##avfun## %s:%d(%s)>>[error]::"#format"\n", __FILE__,__LINE__,__func__,##__VA_ARGS__)

#else

#endif // _WIN32



}

#endif // LogUtil_h__
