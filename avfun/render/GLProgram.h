#ifndef GLProgram_h__
#define GLProgram_h__

#include <AVCommon.h>

namespace avf
{
	namespace render {

		class GLProgram
		{
		public:
			GLProgram(std::string_view vertex_shader, std::string_view fragment_shader);
			~GLProgram();

			void Use();

			void SetBool(const std::string& name, bool value) const;
			void SetInt(const std::string& name, int value) const;
			void SetFloat(const std::string& name, float value) const;
			void SetVec(const std::string& name, float* value, int size) const;

		private:
			struct Impl;
			UP<Impl> _impl;

		};
	}
}

#endif // GLProgram_h__
