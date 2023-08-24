#ifndef AVFUNPLAYER_LOGUTIL_H
#define AVFUNPLAYER_LOGUTIL_H

#include <android/log.h>

#define __FILE__2 (strrchr(__FILE__, '/')?strrchr(__FILE__, '/') + 1:__FILE__)

#define  LOG_TAG    "avfun"

#define LOG_VERBOSE(format, ...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "%s:%d(%s)::"#format"\n",__FILE__2,__LINE__,__func__,##__VA_ARGS__)
#define LOG_DEBUG(format, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "%s:%d(%s)::"#format"\n",__FILE__2,__LINE__,__func__,##__VA_ARGS__)
#define LOG_INFO(format, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s:%d(%s)::"#format"\n",__FILE__2,__LINE__,__func__,##__VA_ARGS__)
#define LOG_WARNING(format, ...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, "%s:%d(%s)::"#format"\n",__FILE__2,__LINE__,__func__,##__VA_ARGS__)
#define LOG_ERROR(format, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "%s:%d(%s)::"#format"\n",__FILE__2,__LINE__,__func__,##__VA_ARGS__)

#define AV_Assert(expression)                                                              \
            if(!expression) {                                                               \
                assert(expression);                                                         \
            }

#endif
