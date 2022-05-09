#ifndef AVCommon_h__
#define AVCommon_h__

#include <string>
#include <memory>
#include <map>
#include <atomic>

namespace avf {

#define PROPERTY(T, _N)     \
    private:                \
        T _##_N;            \
    public:                 \
        void Set##_N(T v){  \
            _##_N = v;      \
        };                  \
        T Get##_N(){        \
            return _##_N;   \
        };

    template<typename T>
    using SP = std::shared_ptr<T>;

    template<typename T>
    using UP = std::unique_ptr<T>;

    template<typename T, typename... Args>
    SP<T> make_sp(Args &&... args) {
        return SP<T>(new T(std::forward<Args>(args)...));
    }

    template<typename T, typename... Args>
    UP<T> make_up(Args &&... args) {
        return UP<T>(new T(std::forward<Args>(args)...));
    }

    template<typename T>
    struct AVFSize {
        T width;
        T height;
    };

    using AVFSizei = AVFSize<int>;
    using AVFSizef = AVFSize<float>;

}


#endif // AVCommon_h__
