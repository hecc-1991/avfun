#ifndef AVFUN_AVPRODUCTER_H
#define AVFUN_AVPRODUCTER_H

#include "AVCommon.h"

namespace avf {

    using ProgressCallback = std::function<int(float)>;

    class AVProducter {
    public:
        AVProducter();

        ~AVProducter();

    public:
        void SetVideoSource(std::string_view filename);

        void SetAudioSource(std::string_view filename);

        int SetOutput(std::string_view filename);

        void SetProgressCallback(ProgressCallback func);

        int Start();

    private:
        struct Impl;
        UP<Impl> _impl;
    };
}


#endif
