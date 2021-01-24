#include "GLProgram.h"
#include "glad/glad.h"
#include "LogUtil.h"

namespace avfun
{
	namespace render {

		struct GLProgram::Impl {

			GLuint id;

			char infoLog[512];

			GLuint create_shader(std::string_view shader, GLenum type);

			void init_shader(std::string_view vertex_shader, std::string_view fragment_shader);

			void release();

			void use();

			void setBool(const std::string& name, bool value) const;
			void setInt(const std::string& name, int value) const;
			void setFloat(const std::string& name, float value) const;
			void setVec(const std::string& name, float* value, int size) const;


		};

		GLuint GLProgram::Impl::create_shader(std::string_view shader, GLenum type) {
			GLuint s = glCreateShader(type);
			auto _shader = shader.data();
			glShaderSource(s, 1, &_shader, NULL);
			glCompileShader(s);
			int success;
			glGetShaderiv(s, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(s, 512, NULL, infoLog);
				LOG_ERROR("ERROR::SHADER::VERTEX::COMPILATION_FAILED :\n%s", infoLog);
				AV_Assert(success);
			};

			return s;
		}

		void GLProgram::Impl::init_shader(std::string_view vertex_shader, std::string_view fragment_shader) {

			GLuint vertex = create_shader(vertex_shader, GL_VERTEX_SHADER);
			GLuint fragment = create_shader(fragment_shader, GL_FRAGMENT_SHADER);

			id = glCreateProgram();
			glAttachShader(id, vertex);
			glAttachShader(id, fragment);
			glLinkProgram(id);

			int success;
			glGetProgramiv(id, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(id, 512, NULL, infoLog);
				LOG_ERROR("ERROR::SHADER::PROGRAM::LINKING_FAILED :\n%s", infoLog);
				AV_Assert(success);
			}

			glDeleteShader(vertex);
			glDeleteShader(fragment);
		}

		void GLProgram::Impl::release() {
			glDeleteProgram(id);
		}

		void GLProgram::Impl::use() {
			glUseProgram(id);
		}

		void GLProgram::Impl::setBool(const std::string& name, bool value) const{
			glUniform1i(glGetUniformLocation(id, name.c_str()), (int)value);
		}
		void GLProgram::Impl::setInt(const std::string& name, int value) const{
			glUniform1i(glGetUniformLocation(id, name.c_str()), value);
		}
		void GLProgram::Impl::setFloat(const std::string& name, float value) const{
			glUniform1f(glGetUniformLocation(id, name.c_str()), value);
		}

		void GLProgram::Impl::setVec(const std::string& name, float* value, int size) const{
			if (size == 2)
				glUniform2fv(glGetUniformLocation(id, name.c_str()), 1, value);
			else if(size == 3)
				glUniform3fv(glGetUniformLocation(id, name.c_str()), 1, value);
			else if (size == 4) 
				glUniform4fv(glGetUniformLocation(id, name.c_str()), 1, value);

		}

		GLProgram::GLProgram(std::string_view vertex_shader, std::string_view fragment_shader):
			_impl(make_up<GLProgram::Impl>()){
			_impl->init_shader(vertex_shader, fragment_shader);
		}

		GLProgram::~GLProgram() {

		}

		void GLProgram::Use() {
			_impl->use();
		}

		void GLProgram::SetBool(const std::string& name, bool value) const{
			_impl->setBool(name, value);
		}
		void GLProgram::SetInt(const std::string& name, int value) const{
			_impl->setInt(name, value);
		}
		void GLProgram::SetFloat(const std::string& name, float value) const{
			_impl->setFloat(name, value);
		}
		void GLProgram::SetVec(const std::string& name, float* value, int size) const {
			_impl->setVec(name, value, size);
		}

	}

}