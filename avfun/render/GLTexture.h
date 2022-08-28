#ifndef AVFUN_GLTEXTURE_H
#define AVFUN_GLTEXTURE_H

#include <AVCommon.h>

namespace avf
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

#endif
