#ifndef GLTexture_h__
#define GLTexture_h__

#include <AVCommon.h>

namespace avfun
{
	namespace render {

		enum class TextureColorSpace :uint32_t
		{
			BGRA = 0,
			RGBA = 1,

		};



		class GLTexture
		{
		public:
			GLTexture(int width, int height, unsigned char* data, TextureColorSpace colorSpace = TextureColorSpace::BGRA);
			~GLTexture();

			unsigned int GetWidth();

			unsigned int GetHeight();

			unsigned int GetTextureId();

			TextureColorSpace GetTextureColorSpace();

		private:
			struct Impl;
			UP<Impl> _impl;
		};
	}
}

#endif // GLTexture_h__
