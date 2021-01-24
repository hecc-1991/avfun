#include "GLTexture.h"
#include "glad/glad.h"

namespace avfun
{
	namespace render {

		struct GLTexture::Impl {

			int _width{ 0 };
			int _height{ 0 };
			TextureColorSpace _colorSpace;

			unsigned int texture;

			void gen_texture(int width, int height, unsigned char* data, TextureColorSpace colorSpace);

			void release();
		};

		void GLTexture::Impl::gen_texture(int width,int height, unsigned char* data, TextureColorSpace colorSpace) {
			
			_width = width;
			_height = height;
			_colorSpace = colorSpace;

			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_CLAMP_TO_BORDER, borderColor);
			int internalformat = colorSpace == TextureColorSpace::BGRA ? GL_BGRA : GL_RGBA;
			glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, internalformat, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		void GLTexture::Impl::release() {
			glDeleteTextures(1, &texture);
		}

		GLTexture::GLTexture(int width, int height, unsigned char* data,TextureColorSpace colorSpace):_impl(make_up<GLTexture::Impl>()) {
			_impl->gen_texture(width, height, data, colorSpace);
		}

		GLTexture::~GLTexture() {
			_impl->release();
		}

		unsigned int GLTexture::GetWidth() {
			return _impl->_width;
		}

		unsigned int GLTexture::GetHeight() {
			return _impl->_height;
		}

		unsigned int GLTexture::GetTextureId() {
			return _impl->texture;
		}

		TextureColorSpace GLTexture::GetTextureColorSpace() {
			return _impl->_colorSpace;
		}

	}
}