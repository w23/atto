#ifndef ATTO_GL_H__DECLARED
#define ATTO_GL_H__DECLARED

#include <atto/platform.h>

#ifdef ATTO_PLATFORM_X11
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glext.h>
#define ATTO_GL_DESKTOP
#endif /* ifdef ATTO_PLATFORM_X11 */

#ifdef ATTO_PLATFORM_RPI
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define ATTO_GL_ES
#endif /* ifdef ATTO_PLATFORM_RPI */

#ifdef ATTO_PLATFORM_WINDOWS
#ifndef ATTO_WINDOWS_H_INCLUDED
#define WIN32_LEAN_AND_MEAN 1
#define NOMINMAX
#include <Windows.h>
#define ATTO_WINDOWS_H_INCLUDED
#endif /* ifndef ATTO_WINDOWS_H_INCLUDED */
#include <GL/gl.h>
#include "glext.h"
#define ATTO_GL_DESKTOP
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#ifdef ATTO_PLATFORM_OSX
#include <OpenGL/gl3.h>
#define ATTO_GL_DESKTOP
#endif

/* Initialize GL stuff, like load GL>=2 procs on Windows */
void aGLInit();

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
} AGLProgramUniform;

typedef GLint AGLProgram;

AGLProgram aGLProgramCreate(const char * const *vertex, const char * const *fragment);
AGLProgram aGLProgramCreateSimple(const char *vertex, const char *fragment);
void aGLProgramUse(AGLProgram program, const AGLProgramUniform *uniforms, unsigned int nuniforms);
#define aGLProgramDestroy(p) do { glDeleteProgram(p); } while(0)

typedef enum {
	AGLTF_Unknown,
	AGLTF_U8_R,
	AGLTF_U8_RA,
	AGLTF_U8_RGB,
	AGLTF_U8_RGBA,
	AGLTF_U565_RGB,
	AGLTF_U5551_RGBA,
	AGLTF_U4444_RGBA
} AGLTextureFormat;

typedef struct {
	GLuint name;
	GLsizei width, height;
	AGLTextureFormat format;
} AGLTexture;

AGLTexture aGLTextureCreate(void);
void aGLTextureUpload(AGLTexture *texture,
		AGLTextureFormat format, int width, int height, const void *pixels);
void aGLTextureBind(const AGLTexture *texture, GLint unit);
#define aGLTextureDestroy(t) do { glDeleteTextures(1, &(t)->name); } while(0)

typedef struct {
	AGLTexture *color;
	int ncolors;
	int depth;
} AGLFramebufferParams;

void aGLFramebufferBind(AGLFramebufferParams params);

typedef enum {
	AGLBT_Vertex = GL_ARRAY_BUFFER,
	AGLBT_Index = GL_ELEMENT_ARRAY_BUFFER
} AGLBufferType;

typedef struct {
	GLuint name;
	AGLBufferType type;
} AGLBuffer;

AGLBuffer aGLBufferCreate(AGLBufferType type);
void aGLBufferUpload(AGLBuffer *buffer, GLsizei size, const void *data);
void aGLBufferBind(const AGLBuffer *buffer);
#define aGLBufferDestroy(fb) do { glDeleteBuffers(1, &fb); } while(0)

typedef struct {
	const char *name;
	const AGLBuffer *buffer;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	const GLvoid *ptr;
} AGLAttribute;

void aGLAttributeBind(const AGLAttribute *attribs, int nattribs,
	const AGLProgram *program);

typedef struct {
	GLenum mode;
	GLsizei count;
	GLint first;
	const AGLBuffer *index_buffer;
	const GLvoid *indices_ptr;
	GLenum index_type;
} AGLPaintParams;

void aGLPaint(AGLPaintParams params);

extern char a_gl_error[];

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

#define ATTO__FUNCLIST_DO(T,N) T gl##N = 0;
ATTO__FUNCLIST
#undef ATTO__FUNCLIST_DO

#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#endif /* ifndef __ATTO_GL_H__DECLARED */

#ifdef ATTO_GL_H_IMPLEMENT
#ifdef __ATTO_GL_H_IMPLEMENTED
#error atto_gl.h must be implemented only once
#endif /* ifdef __ATTO_GL_H_IMPLEMENTED */
#define __ATTO_GL_H_IMPLEMENTED

#ifndef ATTO_ASSERT
#define ATTO_ASSERT(cond) \
	if (!(cond)) { \
		aAppDebugPrintf("ERROR @ %s:%d: (%s) failed", __FILE__, __LINE__, #cond); \
		aAppTerminate(-1); \
	}
#endif /* ifndef ATTO_ASSERT */

#if defined(ATTO_PLATFORM_X11) || defined(ATTO_PLATFORM_RPI) || \
	defined(ATTO_PLATFORM_OSX)
void aGLInit() {
	/* No-op */
}
#endif

#ifdef ATTO_PLATFORM_WINDOWS
static PROC a__check_get_proc_address(const char *name) {
	PROC ret = wglGetProcAddress(name);
	ATTO_ASSERT(ret);
	return ret;
}
void aGLInit() {
#define ATTO__FUNCLIST_DO(T, N) gl##N = (T)a__check_get_proc_address("gl" #N);
	ATTO__FUNCLIST
#undef ATTO__FUNCLIST_DO
}
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#ifndef ATTO_GL_ERROR_BUFFER_SIZE
#define ATTO_GL_ERROR_BUFFER_SIZE 1024
#endif /* ifndef ATTO_GL_ERROR_BUFFER_SIZE */

char a_gl_error[ATTO_GL_ERROR_BUFFER_SIZE];

static GLuint a__GlCreateShader(int type, const char * const *source) {
	int n;
	GLuint shader = glCreateShader(type);

	for (n = 0; source[n] != 0; ++n);

	glShaderSource(shader, n, (const GLchar **)source, 0);
	glCompileShader(shader);

	{
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE) {
			glGetShaderInfoLog(shader, sizeof(a_gl_error), 0, a_gl_error);
			glDeleteShader(shader);
			shader = 0;
		}
	}

	return shader;
}

GLint aGLProgramCreate(const char * const *vertex, const char * const *fragment) {
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	fragment_shader = a__GlCreateShader(GL_FRAGMENT_SHADER, fragment);
	if (fragment_shader == 0)
		return -1;

	vertex_shader = a__GlCreateShader(GL_VERTEX_SHADER, vertex);
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
			glGetProgramInfoLog(program, sizeof(a_gl_error), 0, a_gl_error);
			glDeleteProgram(program);
			return -3;
		}
	}

	return program;
}

GLint aGLProgramCreateSimple(const char *vertex, const char *fragment) {
	const char *vvertex[2] = { 0 };
	const char *vfragment[2] = { 0 };
	vvertex[0] = vertex;
	vfragment[0] = fragment;
	return aGLProgramCreate(vvertex, vfragment);
}

void aGLProgramUse(AGLProgram program,
		const AGLProgramUniform *uniforms, unsigned int nuniforms) {
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

struct {
	GLuint tex_unit;
	GLuint framebuffer;
} a__gl_state;

AGLTexture aGLTextureCreate(void) {
	AGLTexture tex;
	glGenTextures(1, &tex.name);
	tex.width = tex.height = 0;
	tex.format = AGLTF_Unknown;
	return tex;
}

void aGLTextureUpload(AGLTexture *tex,
		AGLTextureFormat aformat, int width, int height, const void *pixels) {
	GLenum internal, format, type;
	switch (aformat) {
		case AGLTF_U8_R:
			internal = format = GL_LUMINANCE; type = GL_UNSIGNED_BYTE; break;
		case AGLTF_U8_RA:
			internal = format = GL_LUMINANCE_ALPHA; type = GL_UNSIGNED_BYTE; break;
		case AGLTF_U8_RGB:
			internal = format = GL_RGB; type = GL_UNSIGNED_BYTE; break;
		case AGLTF_U8_RGBA:
			internal = format = GL_RGBA; type = GL_UNSIGNED_BYTE; break;
		case AGLTF_U565_RGB:
			internal = format = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; break;
		case AGLTF_U5551_RGBA:
			internal = format = GL_RGBA; type = GL_UNSIGNED_SHORT_5_5_5_1; break;
		case AGLTF_U4444_RGBA:
			internal = format = GL_RGBA; type = GL_UNSIGNED_SHORT_4_4_4_4; break;
		default: ATTO_ASSERT(!"Unknown format");
	}
	glBindTexture(GL_TEXTURE_2D, tex->name);
	glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0,
		format, type, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_LINEAR);
	tex->width = width;
	tex->height = height;
	tex->format = format;
}

void aGLTextureBind(const AGLTexture *texture, GLint unit) {
	glActiveTexture(GL_TEXTURE0  + unit);
	glBindTexture(GL_TEXTURE_2D, texture->name);
}

AGLBuffer aGLBufferCreate(AGLBufferType type) {
	AGLBuffer buf;
	glGenBuffers(1, &buf.name);
	buf.type = type;
	return buf;
}

void aGLBufferUpload(AGLBuffer *buffer, GLsizei size, const void *data) {
	glBindBuffer(buffer->type, buffer->name);
	glBufferData(buffer->type, size, data, GL_STATIC_DRAW);
}

void aGLBufferBind(const AGLBuffer *buffer) {
	if (buffer)
		glBindBuffer(buffer->type, buffer->name);
	else {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

void aGLAttributeBind(const AGLAttribute *attribs, int nattribs,
		const AGLProgram *program) {
	int i;
	for (i = 0; i < nattribs; ++i) {
		const AGLAttribute *a = attribs + i;
		GLint loc = glGetAttribLocation(*program, a->name);
		if (loc < 0) continue;
		if (a->buffer)
			aGLBufferBind(a->buffer);
		else
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(loc);
		glVertexAttribPointer(loc, a->size, a->type, a->normalized, a->stride, a->ptr);
	}
}

void aGLPaint(AGLPaintParams params) {
	if (params.first < 0) {
		aGLBufferBind(params.index_buffer);
		glDrawElements(params.mode, params.count, params.index_type, params.indices_ptr);
	} else
		glDrawArrays(params.mode, params.first, params.count);
}

void aGLFramebufferBind(AGLFramebufferParams params) {
	ATTO_ASSERT(!"FIXME: Implement");
}

#endif /* ATTO_GL_H_IMPLEMENT */
