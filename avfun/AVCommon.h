#ifndef AVCommon_h__
#define AVCommon_h__

#include <string>
#include <memory>
#include <map>
#include <atomic> 

namespace avfun {

	template<typename T>
	using SP = std::shared_ptr<T>;

	template<typename T>
	using UP = std::unique_ptr<T>;

	template <typename T, typename... Args>
	SP<T> make_sp(Args&&... args) {
		return SP<T>(new T(std::forward<Args>(args)...));
	}

	template <typename T, typename... Args>
	UP<T> make_up(Args&&... args) {
		return UP<T>(new T(std::forward<Args>(args)...));
	}

}




#endif // AVCommon_h__
