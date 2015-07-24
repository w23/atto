#ifndef ATTO_GL_H__DECLARED
#define ATTO_GL_H__DECLARED

/* common platform detection */
#ifndef ATTO_PLATFORM
#ifdef __linux__
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_LINUX
#define ATTO_PLATFORM_X11
/* FIXME this Linux-only definition should be before ANY header inclusion; this requirement also transcends to the user */
/* required for Linux clock_gettime */
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */
#elif defined(_WIN32)
#define ATTO_PLATFORM_WINDOWS
#elif defined(__MACH__) && defined(__APPLE__)
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_MACH
#define ATTO_PLATFORM_OSX
#else
#error Not ported
#endif
#define ATTO_PLATFORM
#endif /* ifndef ATTO_PLATFORM */
/* end common platform detection */

#ifdef ATTO_PLATFORM_X11
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glext.h>
#endif /* ifdef ATTO_PLATFORM_X11 */

#ifdef ATTO_PLATFORM_WINDOWS
#ifndef ATTO_WINDOWS_H_INCLUDED
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <Windows.h>
#define ATTO_WINDOWS_H_INCLUDED
#endif /* ifndef ATTO_WINDOWS_H_INCLUDED */
#include <GL/gl.h>
#include "glext.h"
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#ifdef ATTO_PLATFORM_OSX
#include <GLUT/GLUT.h>
#include <OpenGL/gl3.h>
#endif

/* Initialize GL stuff, like load GL>=2 procs on Windows */
void aGLInit();

GLint aGLCreateProgram(const char *vertex, const char *fragment);

typedef enum {
	AGLAttribute_Float,
	AGLAttribute_Vec2,
	AGLAttribute_Vec3,
	AGLAttribute_Vec4,
	AGLAttribute_Mat2,
	AGLAttribute_Mat3,
	AGLAttribute_Mat4,
	AGLAttribute_Int,
	AGLAttribute_IVec2,
	AGLAttribute_IVec3,
	AGLAttribute_IVec4
} AGLAttributeType;

typedef struct {
	const char *name;
	AGLAttributeType type;
	const void *pvalue;
	GLsizei count;
} Agl_program_uniform_t;

void aGLUseProgram(GLint program, const Agl_program_uniform_t *uniforms, unsigned int nuniforms);

#ifndef ATTO_GL_ERROR_BUFFER_SIZE
#define ATTO_GL_ERROR_BUFFER_SIZE 1024
#endif /* ifndef ATTO_GL_ERROR_BUFFER_SIZE */

extern char a_gl_error[ATTO_GL_ERROR_BUFFER_SIZE];

#ifdef ATTO_PLATFORM_WINDOWS
#define ATTO__FUNCLIST \
	ATTO__FUNCLIST_DO(PFNGLACTIVETEXTUREPROC, ActiveTexture) \
	ATTO__FUNCLIST_DO(PFNGLCREATESHADERPROC, CreateShader) \
	ATTO__FUNCLIST_DO(PFNGLSHADERSOURCEPROC, ShaderSource) \
	ATTO__FUNCLIST_DO(PFNGLCOMPILESHADERPROC, CompileShader) \
	ATTO__FUNCLIST_DO(PFNGLGENFRAMEBUFFERSPROC, GenFramebuffers) \
	ATTO__FUNCLIST_DO(PFNGLBINDFRAMEBUFFERPROC, BindFramebuffer) \
	ATTO__FUNCLIST_DO(PFNGLFRAMEBUFFERTEXTURE2DPROC, FramebufferTexture2D) \
	ATTO__FUNCLIST_DO(PFNGLUSEPROGRAMPROC, UseProgram) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM1FVPROC, Uniform1fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM2FVPROC, Uniform2fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM3FVPROC, Uniform3fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM4FVPROC, Uniform4fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORMMATRIX2FVPROC, UniformMatrix2fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORMMATRIX3FVPROC, UniformMatrix3fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORMMATRIX4FVPROC, UniformMatrix4fv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM1IVPROC, Uniform1iv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM2IVPROC, Uniform2iv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM3IVPROC, Uniform3iv) \
	ATTO__FUNCLIST_DO(PFNGLUNIFORM4IVPROC, Uniform4iv) \
	ATTO__FUNCLIST_DO(PFNGLCREATEPROGRAMPROC, CreateProgram) \
	ATTO__FUNCLIST_DO(PFNGLATTACHSHADERPROC, AttachShader) \
	ATTO__FUNCLIST_DO(PFNGLBINDATTRIBLOCATIONPROC, BindAttribLocation) \
	ATTO__FUNCLIST_DO(PFNGLLINKPROGRAMPROC, LinkProgram) \
	ATTO__FUNCLIST_DO(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation) \
	ATTO__FUNCLIST_DO(PFNGLDELETESHADERPROC, DeleteShader) \
	ATTO__FUNCLIST_DO(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog) \
	ATTO__FUNCLIST_DO(PFNGLDELETEPROGRAMPROC, DeleteProgram) \
	ATTO__FUNCLIST_DO(PFNGLGETSHADERIVPROC, GetShaderiv) \
	ATTO__FUNCLIST_DO(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog) \
	ATTO__FUNCLIST_DO(PFNGLGETPROGRAMIVPROC, GetProgramiv) \
	ATTO__FUNCLIST_DO(PFNGLCHECKFRAMEBUFFERSTATUSPROC, CheckFramebufferStatus) \
	ATTO__FUNCLIST_DO(PFNGLENABLEVERTEXATTRIBARRAYPROC, EnableVertexAttribArray) \
	ATTO__FUNCLIST_DO(PFNGLVERTEXATTRIBPOINTERPROC, VertexAttribPointer)

#define ATTO__FUNCLIST_DO(T,N) T gl##N;
ATTO__FUNCLIST
#undef ATTO__FUNCLIST_DO

#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#endif /* ifndef __ATTO_GL_H__DECLARED */

#ifdef ATTO_GL_H_IMPLEMENT
#ifdef __ATTO_GL_H_IMPLEMENTED
#error atto_gl.h must be implemented only once
#endif /* ifdef __ATTO_GL_H_IMPLEMENTED */
#define __ATTO_GL_H_IMPLEMENTED

#ifdef ATTO_PLATFORM_X11
void aGLInit() {
	/* No-op */
}
#endif

#ifdef ATTO_PLATFORM_WINDOWS
void aGLInit() {
#define ATTO__FUNCLIST_DO(T, N) gl##N = (T)wglGetProcAddress("gl" #N);
	ATTO__FUNCLIST
#undef ATTO__FUNCLIST_DO
}
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

char a_gl_error[ATTO_GL_ERROR_BUFFER_SIZE];

static GLuint a__GlCreateShader(int type, int n, const char **source) {
	GLuint shader = glCreateShader(type);

	glShaderSource(shader, n, source, NULL);
	glCompileShader(shader);

	{
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE) {
			glGetShaderInfoLog(shader, sizeof(a_gl_error), NULL, a_gl_error);
			glDeleteShader(shader);
			shader = 0;
		}
	}

	return shader;
}

int aGLCreateProgram(const char *vertex, const char *fragment) {
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	fragment_shader = a__GlCreateShader(GL_FRAGMENT_SHADER, 1, &fragment);
	if (fragment_shader == 0)
		return -1;

	vertex_shader = a__GlCreateShader(GL_VERTEX_SHADER, 1, &vertex);
	if (vertex_shader == 0) {
		glDeleteShader(fragment_shader);
		return -2;
	}

	program = glCreateProgram();
	glAttachShader(program, fragment_shader);
	glAttachShader(program, vertex_shader);
	glLinkProgram(program);

	glDeleteShader(fragment_shader);
	glDeleteShader(vertex_shader);

	{
		GLint status;
		glGetProgramiv(program, GL_LINK_STATUS, &status);
		if (status != GL_TRUE) {
			glGetProgramInfoLog(program, sizeof(a_gl_error), NULL, a_gl_error);
			glDeleteProgram(program);
			return -3;
		}
	}

	return program;
}

void aGLUseProgram(int program,
		const Agl_program_uniform_t *uniforms, unsigned int nuniforms) {
	unsigned int i;
	glUseProgram(program);
	for (i = 0; i < nuniforms; ++i) {
		int loc = glGetUniformLocation(program, uniforms[i].name);
		if (loc == -1) continue;
		switch(uniforms[i].type) {
			case AGLAttribute_Float:
				glUniform1fv(loc, uniforms[i].count, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Vec2:
				glUniform2fv(loc, uniforms[i].count, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Vec3:
				glUniform3fv(loc, uniforms[i].count, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Vec4:
				glUniform4fv(loc, uniforms[i].count, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Mat2:
				glUniformMatrix2fv(loc, uniforms[i].count, GL_FALSE, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Mat3:
				glUniformMatrix3fv(loc, uniforms[i].count, GL_FALSE, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Mat4:
				glUniformMatrix4fv(loc, uniforms[i].count, GL_FALSE, (const float*)uniforms[i].pvalue);
				break;
			case AGLAttribute_Int:
				glUniform1iv(loc, uniforms[i].count, (const int*)uniforms[i].pvalue);
				break;
			case AGLAttribute_IVec2:
				glUniform2iv(loc, uniforms[i].count, (const int*)uniforms[i].pvalue);
				break;
			case AGLAttribute_IVec3:
				glUniform3iv(loc, uniforms[i].count, (const int*)uniforms[i].pvalue);
				break;
			case AGLAttribute_IVec4:
				glUniform4iv(loc, uniforms[i].count, (const int*)uniforms[i].pvalue);
				break;
		}
	}
}

#endif /* ATTO_GL_H_IMPLEMENT */
