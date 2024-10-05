#ifndef ATTO_GL_H__DECLARED
#define ATTO_GL_H__DECLARED

#if !defined(ATTO_PLATFORM)
	#include "atto/platform.h"
#endif

#include <stdint.h>

#if !defined(ATTO_GL_HEADERS_INCLUDED)
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
			#ifndef WIN32_LEAN_AND_MEAN
				#define WIN32_LEAN_AND_MEAN 1
			#endif
			#define NOMINMAX
			#include <windows.h>
			#define ATTO_WINDOWS_H_INCLUDED
		#endif /* ifndef ATTO_WINDOWS_H_INCLUDED */
		#include <GL/gl.h>
		#include "khronos/glext.h"
		#define ATTO_GL_DESKTOP
	#endif /* ifdef ATTO_PLATFORM_WINDOWS */

	#ifdef ATTO_PLATFORM_OSX
		#include <OpenGL/gl3.h>
		#define ATTO_GL_DESKTOP
	#endif
#endif /* if !defined(ATTO_GL_HEADERS_INCLUDED) */

/* \todo
 * - utility
 *   + do not depend on atto/platform.h strictly
 *   - assert, print and fatal macro
 *   - argument sanity check
 *   + gl error detection
 *   - profiling
 * - framebuffer
 *   + fb object
 *   - multiple attachments
 *   - depth texture attachment
 * - textures
 *   - parameters (min, mag, wraps)
 *   - cubemap
 *   - compressed
 *   - mipmaps
 *   - download
 *   - partial upload
 *   - RAII
 * - functionality
 *   - copy between buffers/textures
 * - capabilitues
 *   - limits
 *   - features
 *   - extension-based features
 * - optimization
 *   - decrease state changes
 *   - benchmark
 *   - draw-sorting helper
 */

#if defined(__cplusplus)
extern "C" {
#endif

/* Initialize GL stuff, like load GL>=2 procs on Windows */
void aGLInit(void);

/* Textures */

typedef enum {
	AGLTF_Unknown,
	AGLTF_U8_R,
	AGLTF_U8_RA,
	AGLTF_U8_RGB,
	AGLTF_U8_RGBA,
	AGLTF_U565_RGB,
	AGLTF_U5551_RGBA,
	AGLTF_U4444_RGBA,
	AGLTF_F32_RGBA
} AGLTextureFormat;

typedef enum {
	AGLTmF_Nearest = GL_NEAREST,
	AGLTmF_Linear = GL_LINEAR,
	AGLTmF_NearestMipNearest = GL_NEAREST_MIPMAP_NEAREST,
	AGLTmF_NearestMipLinear = GL_NEAREST_MIPMAP_LINEAR,
	AGLTmF_LinearMipNearest = GL_LINEAR_MIPMAP_NEAREST,
	AGLTmF_LinearMipLinear = GL_LINEAR_MIPMAP_LINEAR,
} AGLTextureMinFilter;

typedef enum {
	AGLTMF_Nearest = GL_NEAREST,
	AGLTMF_Linear = GL_LINEAR,
} AGLTextureMagFilter;

typedef enum {
	AGLTW_Clamp = GL_CLAMP_TO_EDGE,
	AGLTW_Repeat = GL_REPEAT,
	AGLTW_MirroredRepeat = GL_MIRRORED_REPEAT
} AGLTextureWrap;

typedef enum {
	AGLTT_NULL,
	AGLTT_1D,
	AGLTT_2D,
	AGLTT_3D,
	AGLTT_2DArray,
} AGLTextureType;

typedef struct {
	AGLTextureType type;
	AGLTextureFormat format;
	GLsizei width, height, depth;
	AGLTextureMinFilter min_filter;
	AGLTextureMagFilter mag_filter;
	AGLTextureWrap wrap_s, wrap_t;
	struct {
		GLuint name;
		GLenum min_filter, mag_filter;
		GLenum wrap_s, wrap_t;
	} _;
	/* \todo unsigned int sequence__; */
} AGLTexture;

enum AGLTextureUpdateFlags {
	// Available only in GLES2+ and GL3+
	AGLTUF_GenerateMipmaps = (1 << 0),
};

typedef struct {
	AGLTextureType type;
	AGLTextureFormat format;
	int x, y, z, width, height, depth;
	uint32_t flags; // Combination of AGLTextureUpdateFlags
	const void *pixels;
} AGLTextureData;

AGLTexture aGLTextureCreate(const AGLTextureData *data);
void aGLTextureUpdate(AGLTexture *texture, const AGLTextureData *data);
#define aGLTextureDestroy(t) \
	do { \
		glDeleteTextures(1, &(t)->_.name); \
		(t)->_.name = 0; \
	} while (0)

/* Shader programs */

/* FIXME rename */
typedef enum {
	AGLAT_Float,
	AGLAT_Vec2,
	AGLAT_Vec3,
	AGLAT_Vec4,
	AGLAT_Mat2,
	AGLAT_Mat3,
	AGLAT_Mat4,
	AGLAT_Int,
	AGLAT_IVec2,
	AGLAT_IVec3,
	AGLAT_IVec4,
	AGLAT_Texture
} AGLAttributeType;

typedef struct {
	const char *name;
	AGLAttributeType type;
	union {
		const GLfloat *pf;
		const GLint *pi;
		const AGLTexture *texture;
	} value;
	GLsizei count;
	struct {
		GLint location;
	} _;
} AGLProgramUniform;

typedef GLint AGLProgram;

AGLProgram aGLProgramCreate(const char *const *vertex, const char *const *fragment);
AGLProgram aGLProgramCreateSimple(const char *vertex, const char *fragment);
#define aGLProgramDestroy(p) \
	do { glDeleteProgram(p); } while (0)
void aGLUniformLocate(AGLProgram program, AGLProgramUniform *uniforms, int count);

/* Array buffers */

typedef enum { AGLBT_Vertex = GL_ARRAY_BUFFER, AGLBT_Index = GL_ELEMENT_ARRAY_BUFFER } AGLBufferType;

typedef struct {
	GLuint name;
	AGLBufferType type;
} AGLBuffer;

AGLBuffer aGLBufferCreate(AGLBufferType type);
void aGLBufferUpload(AGLBuffer *buffer, GLsizei size, const void *data);
#define aGLBufferDestroy(b) \
	do { AGL__CALL(glDeleteBuffers(1, &(b)->name); (b)->name = 0); } while (0)

/* Draw */

typedef struct {
	const char *name;
	const AGLBuffer *buffer;
	GLint size;
	GLenum type;
	GLboolean normalized;
	GLsizei stride;
	const GLvoid *ptr;
	struct {
		GLint location;
	} _;
} AGLAttribute;

void aGLAttributeLocate(AGLProgram program, AGLAttribute *attribs, int count);

typedef enum {
	AGLCM_Disable = 0,
	AGLCM_Front = GL_FRONT,
	AGLCM_Back = GL_BACK, /* default */
	AGLCM_FrontAndBack = GL_FRONT_AND_BACK
} AGLCullMode;

typedef enum {
	AGLFF_Clockwise = GL_CW,
	AGLFF_CounterClockwise = GL_CCW /* default */
} AGLFrontFace;

/* The entire state required for one draw call */
typedef struct {
	AGLProgram program;
	struct {
		const AGLProgramUniform *p;
		unsigned n;
	} uniforms;
	struct {
		const AGLAttribute *p;
		unsigned n;
	} attribs;
	struct {
		GLenum mode;
		GLsizei count;
		GLint first; /* -1 for indexed */
		struct {
			GLenum type;
			union {
				const GLvoid *ptr;
				uintptr_t offset;
			} data;
			const AGLBuffer *buffer;
		} index;
		AGLCullMode cull_mode;
		AGLFrontFace front_face;
	} primitive;
} AGLDrawSource;

typedef enum {
	AGLDF_Never = GL_NEVER,
	AGLDF_Less = GL_LESS, /* default */
	AGLDF_Equal = GL_EQUAL,
	AGLDF_LessOrEqual = GL_LEQUAL,
	AGLDF_Greater = GL_GREATER,
	AGLDF_NotEqual = GL_NOTEQUAL,
	AGLDF_GreaterOrEqual = GL_GEQUAL,
	AGLDF_Always = GL_ALWAYS
} AGLDepthFunc;

typedef enum {
	AGLDM_Disabled = 0, /* default */
	AGLDM_TestOnly = 0x01,
	AGLDM_TestAndWrite = 0x03
} AGLDepthMode;

typedef struct {
	AGLDepthMode mode;
	AGLDepthFunc func;
} AGLDepthParams;

typedef enum {
	AGLBE_Add = GL_FUNC_ADD, /* default */
	AGLBE_Subtract = GL_FUNC_SUBTRACT,
	AGLBE_ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT
} AGLBlendEquation;

typedef enum {
	AGLBF_Zero = GL_ZERO, /* dst default */
	AGLBF_One = GL_ONE, /* src default */
	AGLBF_SrcColor = GL_SRC_COLOR,
	AGLBF_OneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR,
	AGLBF_DstColor = GL_DST_COLOR,
	AGLBF_OneMinusDstColor = GL_ONE_MINUS_DST_COLOR,
	AGLBF_SrcAlpha = GL_SRC_ALPHA,
	AGLBF_OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA,
	AGLBF_DstAlpha = GL_DST_ALPHA,
	AGLBF_OneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA,
	AGLBF_Color = GL_CONSTANT_COLOR,
	AGLBF_OneMinusColor = GL_ONE_MINUS_CONSTANT_COLOR,
	AGLBF_Alpha = GL_CONSTANT_ALPHA,
	AGLBF_OneMinusAlpha = GL_ONE_MINUS_CONSTANT_ALPHA,
	AGLBF_SrcAlphaSaturate = GL_SRC_ALPHA_SATURATE /* src only */
} AGLBlendFunc;

typedef struct {
	int enable;
	struct {
		GLclampf r, g, b, a;
	} color;
	struct {
		AGLBlendEquation rgb, a;
	} equation;
	struct {
		AGLBlendFunc src_rgb, src_a, dst_rgb, dst_a;
	} func;
} AGLBlendParams;

typedef struct {
	AGLBlendParams blend;
	AGLDepthParams depth;
} AGLDrawMerge;

typedef struct {
	const AGLTexture *color;
	struct {
		int enable;
		AGLTexture *texture;
	} depth;
} AGLFramebufferCreate;

typedef struct {
	GLuint name;
	GLuint depth_renderbuffer;
} AGLFramebuffer;

AGLFramebuffer aGLFramebufferCreate(AGLFramebufferCreate params);
void aGLFramebufferDestroy(AGLFramebuffer);

typedef struct {
	struct {
		unsigned x, y, w, h;
	} viewport;
	const AGLFramebuffer *framebuffer;
} AGLDrawTarget;

void aGLDraw(const AGLDrawSource *source, const AGLDrawMerge *merge, const AGLDrawTarget *target);

typedef enum {
	AGLCB_Color = GL_COLOR_BUFFER_BIT,
	AGLCB_Depth = GL_DEPTH_BUFFER_BIT,
	AGLCB_ColorAndDepth = AGLCB_Color | AGLCB_Depth,
	AGLCB_Everything = AGLCB_ColorAndDepth
} AGLClearBits;
typedef struct {
	float r, g, b, a;
	float depth; /* default = 1 */
	AGLClearBits bits;
} AGLClearParams;

void aGLClear(const AGLClearParams *params, const AGLDrawTarget *target);
#if 0
// TODO: need to check whether it's available: gl4, gles3, or GL_ARB_invalidate_subdata
// For that, need to have _features_, either at compile time "I NEED THIS, FAIL WITHOUT", or at runtime -- cheap bools.
void aGLInvalidate(const AGLFramebuffer *fbo);
#endif

/* \todo
typedef struct {
	const char *a_gl_extensions;
} AGLCapabilities;
*/

extern char a_gl_error[];

/***************************************************************************************/
/* IMPLEMENTATION BENEATH */
/***************************************************************************************/

#ifdef ATTO_PLATFORM_WINDOWS
	#define ATTO__GL_FUNCS_LIST(X) \
		X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
		X(PFNGLATTACHSHADERPROC, glAttachShader) \
		X(PFNGLBINDATTRIBLOCATIONPROC, glBindAttribLocation) \
		X(PFNGLBINDBUFFERPROC, glBindBuffer) \
		X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
		X(PFNGLBINDRENDERBUFFERPROC, glBindRenderbuffer) \
		X(PFNGLBLENDCOLORPROC, glBlendColor) \
		X(PFNGLBLENDEQUATIONPROC, glBlendEquation) \
		X(PFNGLBLENDEQUATIONSEPARATEPROC, glBlendEquationSeparate) \
		X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
		X(PFNGLBUFFERDATAPROC, glBufferData) \
		X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
		X(PFNGLCLEARDEPTHFPROC, glClearDepthf) \
		X(PFNGLCOMPILESHADERPROC, glCompileShader) \
		X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
		X(PFNGLCREATESHADERPROC, glCreateShader) \
		X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers) \
		X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
		X(PFNGLDELETERENDERBUFFERSPROC, glDeleteRenderbuffers) \
		X(PFNGLDELETESHADERPROC, glDeleteShader) \
		X(PFNGLDISABLEVERTEXATTRIBARRAYPROC, glDisableVertexAttribArray) \
		X(PFNGLDRAWBUFFERSPROC, glDrawBuffers) \
		X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
		X(PFNGLFRAMEBUFFERRENDERBUFFERPROC, glFramebufferRenderbuffer) \
		X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
		X(PFNGLGENBUFFERSPROC, glGenBuffers) \
		X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
		X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
		X(PFNGLGENRENDERBUFFERSPROC, glGenRenderbuffers) \
		X(PFNGLGETATTRIBLOCATIONPROC, glGetAttribLocation) \
		X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
		X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
		X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
		X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
		X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
		X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
		X(PFNGLRENDERBUFFERSTORAGEPROC, glRenderbufferStorage) \
		X(PFNGLSHADERSOURCEPROC, glShaderSource) \
		X(PFNGLTEXIMAGE3DPROC, glTexImage3D) \
		X(PFNGLTEXSUBIMAGE3DPROC, glTexSubImage3D) \
		X(PFNGLUNIFORM1FPROC, glUniform1f) \
		X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
		X(PFNGLUNIFORM1IPROC, glUniform1i) \
		X(PFNGLUNIFORM1IVPROC, glUniform1iv) \
		X(PFNGLUNIFORM2FPROC, glUniform2f) \
		X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
		X(PFNGLUNIFORM2IVPROC, glUniform2iv) \
		X(PFNGLUNIFORM3FPROC, glUniform3f) \
		X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
		X(PFNGLUNIFORM3IVPROC, glUniform3iv) \
		X(PFNGLUNIFORM4FPROC, glUniform4f) \
		X(PFNGLUNIFORM4FVPROC, glUniform4fv) \
		X(PFNGLUNIFORM4IVPROC, glUniform4iv) \
		X(PFNGLUNIFORMMATRIX2FVPROC, glUniformMatrix2fv) \
		X(PFNGLUNIFORMMATRIX3FVPROC, glUniformMatrix3fv) \
		X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
		X(PFNGLUSEPROGRAMPROC, glUseProgram) \
		X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \

#define ATTO__DECLARE_FUNC_EXTERN(T_, N_) extern T_ N_;
ATTO__GL_FUNCS_LIST(ATTO__DECLARE_FUNC_EXTERN)
#undef ATTO__DECLARE_FUNC_EXTERN
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ifndef ATTO_GL_H__DECLARED */

#ifdef ATTO_GL_H_IMPLEMENT
#ifdef ATTO__GL_H_IMPLEMENTED
	#error atto_gl.h must be implemented only once
#endif /* ifdef ATTO__GL_H_IMPLEMENTED */
#define ATTO__GL_H_IMPLEMENTED

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef AGL_PRINTFLN
#define AGL_PRINTFLN(msg, ...) aAppDebugPrintf("[agl] " msg, ##__VA_ARGS__)
#endif

#ifndef ATTO_ASSERT
	#define ATTO_ASSERT(cond) \
		if (!(cond)) { \
			AGL_PRINTFLN("ERROR @ %s:%d: (%s) failed", __FILE__, __LINE__, #cond); \
			aAppTerminate(-1); \
		}
#endif /* ifndef ATTO_ASSERT */

#ifdef ATTO_GL_PROFILE_FUNC
	#define ATTO_GL_PROFILE_PREAMBLE const ATimeUs start = aAppTime();
	#define ATTO_GL_PROFILE_START const ATimeUs agl_profile_start_ = aAppTime();
	#define ATTO_GL_PROFILE_END ATTO_GL_PROFILE_FUNC(__FUNCTION__, aAppTime() - agl_profile_start_);
	#define ATTO_GL_PROFILE_END_NAME(name) ATTO_GL_PROFILE_FUNC(name, aAppTime() - agl_profile_start_);
#else
	#define ATTO_GL_PROFILE_PREAMBLE
	#define ATTO_GL_PROFILE_FUNC(...)
	#define ATTO_GL_PROFILE_START
	#define ATTO_GL_PROFILE_END
	#define ATTO_GL_PROFILE_END_NAME(name)
#endif

#ifndef ATTO_GL_DEBUG
	#define AGL__CALL(f) (f)
#else
	#include <stdlib.h> /* abort() */
static void a__GlPrintError(const char *message, GLenum error) {
	const char *errstr = "UNKNOWN";
	switch (error) {
	case GL_INVALID_ENUM: errstr = "GL_INVALID_ENUM"; break;
	case GL_INVALID_VALUE: errstr = "GL_INVALID_VALUE"; break;
	case GL_INVALID_OPERATION: errstr = "GL_INVALID_OPERATION"; break;
	#ifdef GL_STACK_OVERFLOW
	case GL_STACK_OVERFLOW: errstr = "GL_STACK_OVERFLOW"; break;
	#endif
	#ifdef GL_STACK_UNDERFLOW
	case GL_STACK_UNDERFLOW: errstr = "GL_STACK_UNDERFLOW"; break;
	#endif
	case GL_OUT_OF_MEMORY: errstr = "GL_OUT_OF_MEMORY"; break;
	#ifdef GL_TABLE_TOO_LARGE
	case GL_TABLE_TOO_LARGE: errstr = "GL_TABLE_TOO_LARGE"; break;
	#endif
	};
	AGL_PRINTFLN("%s %s (%#x)", message, errstr, error);
}
	#define ATTO__GL_STR__(s) #s
	#define ATTO__GL_STR(s) ATTO__GL_STR__(s)
	#ifdef ATTO_GL_TRACE
		#define ATTO_GL_TRACE_PRINT AGL_PRINTFLN
	#else
		#define ATTO_GL_TRACE_PRINT(...)
	#endif
	#define AGL__CALL(f) \
		do { \
			ATTO_GL_TRACE_PRINT("%s", #f); \
			ATTO_GL_PROFILE_PREAMBLE \
			f; \
			ATTO_GL_PROFILE_FUNC(#f, aAppTime() - start); \
			const GLenum glerror = glGetError(); \
			if (glerror != GL_NO_ERROR) { \
				a__GlPrintError(__FILE__ ":" ATTO__GL_STR(__LINE__) ": " #f " returned ", glerror); \
				abort(); \
			} \
		} while (0)
#endif /* ATTO_GL_DEBUG */

#ifndef ATTO_GL_ERROR_BUFFER_SIZE
	#define ATTO_GL_ERROR_BUFFER_SIZE 1024
#endif /* ifndef ATTO_GL_ERROR_BUFFER_SIZE */

char a_gl_error[ATTO_GL_ERROR_BUFFER_SIZE];

#ifndef ATTO_GL_MAX_ATTRIBS
	#define ATTO_GL_MAX_ATTRIBS 8
#endif

typedef struct {
	unsigned int vertices;
	unsigned int triangles;
	unsigned int draw_calls;
} AGLStats;

static struct {
	AGLProgram program;
	AGLCullMode cull_mode;
	AGLFrontFace front_face;
	AGLBlendParams blend;
	AGLDepthParams depth;
	unsigned attribs_serial;
	struct {
		GLint buffer;
		AGLAttribute attrib;
		unsigned serial;
	} attribs[ATTO_GL_MAX_ATTRIBS];

	struct {
		GLuint binding;
	} framebuffer;

	struct {
		unsigned x, y, w, h;
	} viewport;

	AGLStats stats;
} a__gl_state;

static GLuint a__GLCreateShader(int type, const char *const *source);
static void a__GLProgramBind(AGLProgram program, const AGLProgramUniform *uniforms, int nuniforms);
static void a__GLTextureBind(const AGLTexture *texture, GLint unit);
static void a__GLAttribsBind(const AGLAttribute *attrs, int nattrs);
static void a__GLCullingBind(AGLCullMode cull, AGLFrontFace front);
static void a__GLDepthBind(AGLDepthParams depth);
static void a__GLBlendBind(const AGLBlendParams *blend);
static void a__GLFramebufferBind(const AGLFramebuffer *fbo);
static void a__GLTargetBind(const AGLDrawTarget *target);

#ifdef ATTO_PLATFORM_WINDOWS
#define ATTO__DECLARE_FUNC(T_, N_) T_ N_ = 0;
ATTO__GL_FUNCS_LIST(ATTO__DECLARE_FUNC)
#undef ATTO__DECLARE_FUNC

static PROC a__check_get_proc_address(const char *name) {
	PROC ret = wglGetProcAddress(name);
	ATTO_ASSERT(ret);
	return ret;
}
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#ifdef ATTO_GL_PRINT_LIMITS
#define A__GL_PRINT_LIMIT(enum, num) a__GlGetAndPrintInteger(enum, #enum, num)
static void a__GlGetAndPrintInteger(GLenum pname, const char *name, int count) {
	GLint params[2] = {0};
	glGetIntegerv(pname, params);
	if (count == 1)
		AGL_PRINTFLN("%s: %d", name, params[0]);
	else if (count == 2)
		AGL_PRINTFLN("%s: %d, %d", name, params[0], params[1]);
}
#endif

void aGLInit(void) {
#ifdef ATTO_PLATFORM_WINDOWS
#define ATTO__GET_FUNC(T_, N_) N_ = (T_)a__check_get_proc_address(#N_);
	ATTO__GL_FUNCS_LIST(ATTO__GET_FUNC)
#undef ATTO__GET_FUNC
#endif /* ifdef ATTO_PLATFORM_WINDOWS */

#ifndef ATTO_GL_DONT_PRINT_INFO
	AGL_PRINTFLN("GL_VENDOR: %s", glGetString(GL_VENDOR));
	AGL_PRINTFLN("GL_RENDERER: %s", glGetString(GL_RENDERER));
	AGL_PRINTFLN("GL_VERSION: %s", glGetString(GL_VERSION));
	AGL_PRINTFLN("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));
	AGL_PRINTFLN("GL_EXTENSIONS: %s", glGetString(GL_EXTENSIONS));
#endif

#ifdef ATTO_GL_PRINT_LIMITS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_CUBE_MAP_TEXTURE_SIZE, 1);
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_UNIFORM_VECTORS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_RENDERBUFFER_SIZE, 1);
	A__GL_PRINT_LIMIT(GL_MAX_TEXTURE_IMAGE_UNITS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_TEXTURE_SIZE, 1);
	A__GL_PRINT_LIMIT(GL_MAX_VARYING_VECTORS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_ATTRIBS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_UNIFORM_VECTORS, 1);
	A__GL_PRINT_LIMIT(GL_MAX_VIEWPORT_DIMS, 2);
	A__GL_PRINT_LIMIT(GL_NUM_COMPRESSED_TEXTURE_FORMATS, 1);
	A__GL_PRINT_LIMIT(GL_NUM_SHADER_BINARY_FORMATS, 1);

#ifdef GL_MAX_3D_TEXTURE_SIZE
	A__GL_PRINT_LIMIT(GL_MAX_3D_TEXTURE_SIZE, 1);
#endif
#ifdef GL_MAX_ARRAY_TEXTURE_LAYERS
	A__GL_PRINT_LIMIT(GL_MAX_ARRAY_TEXTURE_LAYERS, 1);
#endif
#ifdef GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS
	A__GL_PRINT_LIMIT(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS, 1);
#endif
#ifdef GL_MAX_COLOR_ATTACHMENTS
	A__GL_PRINT_LIMIT(GL_MAX_COLOR_ATTACHMENTS, 1);
#endif
#ifdef GL_MAX_COLOR_TEXTURE_SAMPLES
	A__GL_PRINT_LIMIT(GL_MAX_COLOR_TEXTURE_SAMPLES, 1);
#endif
#ifdef GL_MAX_COMBINED_ATOMIC_COUNTERS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_ATOMIC_COUNTERS, 1);
#endif
#ifdef GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, 1);
#endif
#ifdef GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_FRAGMENT_UNIFORM_COMPONENTS, 1);
#endif
#ifdef GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, 1);
#endif
#ifdef GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, 1);
#endif
#ifdef GL_MAX_COMBINED_UNIFORM_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_UNIFORM_BLOCKS, 1);
#endif
#ifdef GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_COMBINED_VERTEX_UNIFORM_COMPONENTS, 1);
#endif
#ifdef GL_MAX_COMPUTE_ATOMIC_COUNTERS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_ATOMIC_COUNTERS, 1);
#endif
#ifdef GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, 1);
#endif
#ifdef GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, 1);
#endif
#ifdef GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, 1);
#endif
#ifdef GL_MAX_COMPUTE_UNIFORM_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_UNIFORM_BLOCKS, 1);
#endif
#ifdef GL_MAX_COMPUTE_UNIFORM_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, 1);
#endif
#ifdef GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS
	A__GL_PRINT_LIMIT(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, 1);
#endif
#ifdef GL_MAX_CUBE_MAP_TEXTURE_SIZE
	A__GL_PRINT_LIMIT(GL_MAX_CUBE_MAP_TEXTURE_SIZE, 1);
#endif
#ifdef GL_MAX_DRAW_BUFFERS
	A__GL_PRINT_LIMIT(GL_MAX_DRAW_BUFFERS, 1);
#endif
#ifdef GL_MAX_ELEMENT_INDEX
	A__GL_PRINT_LIMIT(GL_MAX_ELEMENT_INDEX, 1);
#endif
#ifdef GL_MAX_ELEMENTS_INDICES
	A__GL_PRINT_LIMIT(GL_MAX_ELEMENTS_INDICES, 1);
#endif
#ifdef GL_MAX_ELEMENTS_VERTICES
	A__GL_PRINT_LIMIT(GL_MAX_ELEMENTS_VERTICES, 1);
#endif
#ifdef GL_MAX_FRAGMENT_ATOMIC_COUNTERS
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_ATOMIC_COUNTERS, 1);
#endif
#ifdef GL_MAX_FRAGMENT_INPUT_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_INPUT_COMPONENTS, 1);
#endif
#ifdef GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, 1);
#endif
#ifdef GL_MAX_FRAGMENT_UNIFORM_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, 1);
#endif
#ifdef GL_MAX_FRAGMENT_UNIFORM_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, 1);
#endif
#ifdef GL_MAX_FRAGMENT_UNIFORM_VECTORS
	A__GL_PRINT_LIMIT(GL_MAX_FRAGMENT_UNIFORM_VECTORS, 1);
#endif
#ifdef GL_MAX_FRAMEBUFFER_HEIGHT
	A__GL_PRINT_LIMIT(GL_MAX_FRAMEBUFFER_HEIGHT, 1);
#endif
#ifdef GL_MAX_FRAMEBUFFER_SAMPLES
	A__GL_PRINT_LIMIT(GL_MAX_FRAMEBUFFER_SAMPLES, 1);
#endif
#ifdef GL_MAX_FRAMEBUFFER_WIDTH
	A__GL_PRINT_LIMIT(GL_MAX_FRAMEBUFFER_WIDTH, 1);
#endif
#ifdef GL_MAX_INTEGER_SAMPLES
	A__GL_PRINT_LIMIT(GL_MAX_INTEGER_SAMPLES, 1);
#endif
#ifdef GL_MAX_PROGRAM_TEXEL_OFFSET
	A__GL_PRINT_LIMIT(GL_MAX_PROGRAM_TEXEL_OFFSET, 1);
#endif
#ifdef GL_MAX_RENDERBUFFER_SIZE
	A__GL_PRINT_LIMIT(GL_MAX_RENDERBUFFER_SIZE, 1);
#endif
#ifdef GL_MAX_SAMPLE_MASK_WORDS
	A__GL_PRINT_LIMIT(GL_MAX_SAMPLE_MASK_WORDS, 1);
#endif
#ifdef GL_MAX_SAMPLES
	A__GL_PRINT_LIMIT(GL_MAX_SAMPLES, 1);
#endif
#ifdef GL_MAX_SERVER_WAIT_TIMEOUT
	A__GL_PRINT_LIMIT(GL_MAX_SERVER_WAIT_TIMEOUT, 1);
#endif
#ifdef GL_MAX_SHADER_STORAGE_BLOCK_SIZE
	A__GL_PRINT_LIMIT(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, 1);
#endif
#ifdef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
	A__GL_PRINT_LIMIT(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, 1);
#endif
#ifdef GL_MAX_TEXTURE_IMAGE_UNITS
	A__GL_PRINT_LIMIT(GL_MAX_TEXTURE_IMAGE_UNITS, 1);
#endif
#ifdef GL_MAX_TEXTURE_LOD_BIAS
	A__GL_PRINT_LIMIT(GL_MAX_TEXTURE_LOD_BIAS, 1);
#endif
#ifdef GL_MAX_TEXTURE_SIZE
	A__GL_PRINT_LIMIT(GL_MAX_TEXTURE_SIZE, 1);
#endif
#ifdef GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS, 1);
#endif
#ifdef GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS
	A__GL_PRINT_LIMIT(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS, 1);
#endif
#ifdef GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS, 1);
#endif
#ifdef GL_MAX_UNIFORM_BLOCK_SIZE
	A__GL_PRINT_LIMIT(GL_MAX_UNIFORM_BLOCK_SIZE, 1);
#endif
#ifdef GL_MAX_UNIFORM_BUFFER_BINDINGS
	A__GL_PRINT_LIMIT(GL_MAX_UNIFORM_BUFFER_BINDINGS, 1);
#endif
#ifdef GL_MAX_UNIFORM_LOCATIONS
	A__GL_PRINT_LIMIT(GL_MAX_UNIFORM_LOCATIONS, 1);
#endif
#ifdef GL_MAX_VARYING_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_VARYING_COMPONENTS, 1);
#endif
#ifdef GL_MAX_VARYING_VECTORS
	A__GL_PRINT_LIMIT(GL_MAX_VARYING_VECTORS, 1);
#endif
#ifdef GL_MAX_VERTEX_ATOMIC_COUNTERS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_ATOMIC_COUNTERS, 1);
#endif
#ifdef GL_MAX_VERTEX_ATTRIB_BINDINGS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_ATTRIB_BINDINGS, 1);
#endif
#ifdef GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, 1);
#endif
#ifdef GL_MAX_VERTEX_ATTRIBS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_ATTRIBS, 1);
#endif
#ifdef GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, 1);
#endif
#ifdef GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, 1);
#endif
#ifdef GL_MAX_VERTEX_OUTPUT_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_OUTPUT_COMPONENTS, 1);
#endif
#ifdef GL_MAX_VERTEX_UNIFORM_BLOCKS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_UNIFORM_BLOCKS, 1);
#endif
#ifdef GL_MAX_VERTEX_UNIFORM_COMPONENTS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_UNIFORM_COMPONENTS, 1);
#endif
#ifdef GL_MAX_VERTEX_UNIFORM_VECTORS
	A__GL_PRINT_LIMIT(GL_MAX_VERTEX_UNIFORM_VECTORS, 1);
#endif

// INDEXED
//GL_MAX_COMPUTE_WORK_GROUP_COUNT
//GL_MAX_COMPUTE_WORK_GROUP_SIZE

	glGetError();
#endif

	/* default initial GL state */
	a__gl_state.cull_mode = AGLCM_Disable;
	a__gl_state.front_face = AGLFF_CounterClockwise;
	a__gl_state.blend.equation.rgb = a__gl_state.blend.equation.a = AGLBE_Add;
	a__gl_state.blend.func.src_rgb = a__gl_state.blend.func.src_a = AGLBF_One;
	a__gl_state.blend.func.dst_rgb = a__gl_state.blend.func.dst_a = AGLBF_Zero;
	a__gl_state.depth.mode = AGLDM_Disabled;
	a__gl_state.depth.func = AGLDF_Less;

	a__gl_state.attribs_serial = 0;
	for (int i = 0; i < ATTO_GL_MAX_ATTRIBS; ++i) {
		a__gl_state.attribs[i].buffer = -1;
		a__gl_state.attribs[i].serial = 0;
	}
}

GLint aGLProgramCreate(const char *const *vertex, const char *const *fragment) {
	GLuint program;
	GLuint vertex_shader, fragment_shader;
	fragment_shader = a__GLCreateShader(GL_FRAGMENT_SHADER, fragment);
	if (fragment_shader == 0)
		return -1;

	vertex_shader = a__GLCreateShader(GL_VERTEX_SHADER, vertex);
	if (vertex_shader == 0) {
		AGL__CALL(glDeleteShader(fragment_shader));
		return -2;
	}

	program = glCreateProgram();
	AGL__CALL(glAttachShader(program, fragment_shader));
	AGL__CALL(glAttachShader(program, vertex_shader));
	AGL__CALL(glLinkProgram(program));

	AGL__CALL(glDeleteShader(fragment_shader));
	AGL__CALL(glDeleteShader(vertex_shader));

	{
		GLint status;
		AGL__CALL(glGetProgramiv(program, GL_LINK_STATUS, &status));
		if (status != GL_TRUE) {
			AGL__CALL(glGetProgramInfoLog(program, sizeof(a_gl_error), 0, a_gl_error));
			AGL__CALL(glDeleteProgram(program));
			return -3;
		}
	}

	return program;
}

GLint aGLProgramCreateSimple(const char *vertex, const char *fragment) {
	const char *vvertex[2] = {0};
	const char *vfragment[2] = {0};
	vvertex[0] = vertex;
	vfragment[0] = fragment;
	return aGLProgramCreate(vvertex, vfragment);
}

void aGLUniformLocate(AGLProgram program, AGLProgramUniform *uniforms, int count) {
	for (int i = 0; i < count; ++i)
		uniforms[i]._.location = glGetUniformLocation(program, uniforms[i].name);
}

void aGLAttributeLocate(AGLProgram program, AGLAttribute *attribs, int count) {
	for (int i = 0; i < count; ++i)
		attribs[i]._.location = glGetAttribLocation(program, attribs[i].name);
}

AGLTexture aGLTextureCreate(const AGLTextureData *data) {
	AGLTexture tex = {
		.type = data->type,
		.format = AGLTF_Unknown,
		._.mag_filter = tex._.min_filter = (GLenum)-1,
		._.wrap_s = tex._.wrap_t = (GLenum)-1,
		.mag_filter = AGLTMF_Linear,
		.min_filter = (data->flags & AGLTUF_GenerateMipmaps)
			? AGLTmF_LinearMipLinear
			: AGLTmF_Linear,
		.wrap_s = tex.wrap_t = AGLTW_Repeat,
	};

	AGL__CALL(glGenTextures(1, &tex._.name));
	aGLTextureUpdate(&tex, data);
	return tex;
}

struct A__GLTextureFormat {
	GLenum internal, format, type;
};

static struct A__GLTextureFormat getTextureFormat(AGLTextureFormat aformat) {
	struct A__GLTextureFormat tf = {0};
	switch (aformat) {
	case AGLTF_U8_R:
		tf.internal = tf.format = GL_LUMINANCE;
		tf.type = GL_UNSIGNED_BYTE;
		break;
	case AGLTF_U8_RA:
		tf.internal = tf.format = GL_LUMINANCE_ALPHA;
		tf.type = GL_UNSIGNED_BYTE;
		break;
	case AGLTF_U8_RGB:
		tf.internal = tf.format = GL_RGB;
		tf.type = GL_UNSIGNED_BYTE;
		break;
	case AGLTF_U8_RGBA:
		tf.internal = tf.format = GL_RGBA;
		tf.type = GL_UNSIGNED_BYTE;
		break;
	case AGLTF_U565_RGB:
		tf.internal = tf.format = GL_RGB;
		tf.type = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case AGLTF_U5551_RGBA:
		tf.internal = tf.format = GL_RGBA;
		tf.type = GL_UNSIGNED_SHORT_5_5_5_1;
		break;
	case AGLTF_U4444_RGBA:
		tf.internal = tf.format = GL_RGBA;
		tf.type = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
	case AGLTF_F32_RGBA:
#ifdef GL_RGBA32F
		tf.internal = GL_RGBA32F;
		tf.format = GL_RGBA;
		tf.type = GL_FLOAT;
		break;
#endif
	case AGLTF_Unknown: ATTO_ASSERT(!"Unknown format");
	}
	ATTO_ASSERT(tf.internal != 0);
	return tf;
}

typedef void (a__gl_texture_upload_func)(AGLTexture *tex, const AGLTextureData *data, GLenum binding, struct A__GLTextureFormat tf);

static void a__GLTexureUpload1D(AGLTexture *tex, const AGLTextureData *data, GLenum binding, struct A__GLTextureFormat tf) {
	const int maxwidth = data->x + data->width;

	const int expand = maxwidth > tex->width;
	const int is_subimage = data->x > 0;

	ATTO_ASSERT(binding == GL_TEXTURE_1D);

	if (expand) {
		AGL__CALL(glTexImage1D(binding, 0, tf.internal, maxwidth, 0,
			tf.format, tf.type, is_subimage ? NULL : data->pixels));
		tex->width = maxwidth;
	}

	if (is_subimage) {
		ATTO_ASSERT(data->pixels);
		AGL__CALL(glTexSubImage1D(binding, 0,
			data->x, data->width,
			tf.format, tf.type, data->pixels));
	}
}

static void a__GLTexureUpload2D(AGLTexture *tex, const AGLTextureData *data, GLenum binding, struct A__GLTextureFormat tf) {
	const int maxwidth = data->x + data->width;
	const int maxheight = data->y + data->height;

	const int expand = (maxwidth > tex->width) || (maxheight > tex->height);
	const int is_subimage = (data->x > 0 || data->y > 0);

	ATTO_ASSERT(binding == GL_TEXTURE_2D);

	if (expand) {
		AGL__CALL(glTexImage2D(binding, 0, tf.internal, maxwidth, maxheight, 0,
			tf.format, tf.type, is_subimage ? NULL : data->pixels));
		tex->width = maxwidth;
		tex->height = maxheight;
	}

	if (is_subimage) {
		ATTO_ASSERT(data->pixels);
		AGL__CALL(glTexSubImage2D(binding, 0,
			data->x, data->y, data->width, data->height,
			tf.format, tf.type, data->pixels));
	}
}

static void a__GLTexureUpload3D(AGLTexture *tex, const AGLTextureData *data, GLenum binding, struct A__GLTextureFormat tf) {
	const int maxwidth = data->x + data->width;
	const int maxheight = data->y + data->height;
	const int maxdepth = data->z + data->depth;

	const int expand = (maxwidth > tex->width)
		|| (maxheight > tex->height)
		|| (maxdepth > tex->depth);
	const int is_subimage = (data->x > 0 || data->y > 0 || data->z > 0);

	ATTO_ASSERT(binding == GL_TEXTURE_3D || binding == GL_TEXTURE_2D_ARRAY);

	if (expand) {
		AGL__CALL(glTexImage3D(binding, 0, tf.internal,
			maxwidth, maxheight, maxdepth, 0,
			tf.format, tf.type,
			is_subimage ? NULL : data->pixels));
		tex->width = maxwidth;
		tex->height = maxheight;
		tex->depth = maxdepth;
	}

	if (is_subimage) {
		ATTO_ASSERT(data->pixels);
		AGL__CALL(glTexSubImage3D(binding, 0,
			data->x, data->y, data->z, data->width, data->height, data->depth,
			tf.format, tf.type, data->pixels));
	}
}

void aGLTextureUpdate(AGLTexture *tex, const AGLTextureData *data) {
	struct A__GLTextureFormat tf = getTextureFormat(data->format);

	a__gl_texture_upload_func *upload_func = NULL;
	GLenum binding = 0;

	switch (data->type) {
		case AGLTT_1D:
			upload_func = &a__GLTexureUpload1D;
			binding = GL_TEXTURE_1D;
			break;
		case AGLTT_2D:
			upload_func = &a__GLTexureUpload2D;
			binding = GL_TEXTURE_2D;
			break;
		case AGLTT_3D:
			upload_func = &a__GLTexureUpload3D;
			binding = GL_TEXTURE_3D;
			break;
		case AGLTT_2DArray:
			upload_func = &a__GLTexureUpload3D;
			binding = GL_TEXTURE_2D_ARRAY;
			break;
		case AGLTT_NULL: ATTO_ASSERT(!"Invalid texture type");
	}
	ATTO_ASSERT(upload_func);

	// TODO track bindings properly
	AGL__CALL(glBindTexture(binding, tex->_.name));

	upload_func(tex, data, binding, tf);

	if (data->pixels && (data->flags & AGLTUF_GenerateMipmaps))
		AGL__CALL(glGenerateMipmap(binding));

	tex->type = data->type;
	tex->format = data->format;
}

AGLBuffer aGLBufferCreate(AGLBufferType type) {
	AGLBuffer buf;
	AGL__CALL(glGenBuffers(1, &buf.name));
	buf.type = type;
	return buf;
}

void aGLBufferUpload(AGLBuffer *buffer, GLsizei size, const void *data) {
	AGL__CALL(glBindBuffer(buffer->type, buffer->name));
	AGL__CALL(glBufferData(buffer->type, size, data, GL_STATIC_DRAW));
}

void aGLDraw(const AGLDrawSource *src, const AGLDrawMerge *merge, const AGLDrawTarget *target) {
	ATTO_GL_PROFILE_PREAMBLE
	a__GLTargetBind(target);

	a__GLDepthBind(merge->depth);
	a__GLBlendBind(&merge->blend);

	a__GLProgramBind(src->program, src->uniforms.p, src->uniforms.n);
	a__GLAttribsBind(src->attribs.p, src->attribs.n);
	a__GLCullingBind(src->primitive.cull_mode, src->primitive.front_face);

	if (src->primitive.index.buffer || src->primitive.index.data.ptr) {
		if (src->primitive.index.buffer) {
			/* FIXME store binding in state */
			AGL__CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, src->primitive.index.buffer->name));
		} else {
			AGL__CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
		}

		AGL__CALL(glDrawElements(
			src->primitive.mode, src->primitive.count, src->primitive.index.type, src->primitive.index.data.ptr));
	} else
		AGL__CALL(glDrawArrays(src->primitive.mode, src->primitive.first, src->primitive.count));
	ATTO_GL_PROFILE_FUNC("aGLDraw", aAppTime() - start);
}

void aGLClear(const AGLClearParams *params, const AGLDrawTarget *target) {
	a__GLTargetBind(target);

	AGL__CALL(glClearColor(params->r, params->g, params->b, params->a));
	AGL__CALL(glClearDepthf(params->depth));
	AGL__CALL(glClear(params->bits));
}

static GLuint a__GLCreateShader(int type, const char *const *source) {
	int n;
	GLuint shader = glCreateShader(type);

	for (n = 0; source[n] != 0; ++n)
		;

	AGL__CALL(glShaderSource(shader, n, (const GLchar **)source, 0));
	AGL__CALL(glCompileShader(shader));

	{
		GLint status;
		AGL__CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &status));
		if (status != GL_TRUE) {
			AGL__CALL(glGetShaderInfoLog(shader, sizeof(a_gl_error), 0, a_gl_error));
			AGL__CALL(glDeleteShader(shader));
			shader = 0;
		}
	}

	return shader;
}

void a__GLProgramBind(AGLProgram program, const AGLProgramUniform *uniforms, int nuniforms) {
	ATTO_GL_PROFILE_START
	int i, texture_unit = 0;
	AGL__CALL(glUseProgram(program));
	for (i = 0; i < nuniforms; ++i) {
		ATTO_GL_PROFILE_START
		const int loc = uniforms[i]._.location;
		if (loc == -1) { /*AGL_PRINTFLN("Skipping %s", uniforms[i].name);*/
			continue;
		}
		switch (uniforms[i].type) {
		case AGLAT_Float: AGL__CALL(glUniform1fv(loc, uniforms[i].count, uniforms[i].value.pf)); break;
		case AGLAT_Vec2: AGL__CALL(glUniform2fv(loc, uniforms[i].count, uniforms[i].value.pf)); break;
		case AGLAT_Vec3: AGL__CALL(glUniform3fv(loc, uniforms[i].count, uniforms[i].value.pf)); break;
		case AGLAT_Vec4: AGL__CALL(glUniform4fv(loc, uniforms[i].count, uniforms[i].value.pf)); break;
		case AGLAT_Mat2: AGL__CALL(glUniformMatrix2fv(loc, uniforms[i].count, GL_FALSE, uniforms[i].value.pf)); break;
		case AGLAT_Mat3: AGL__CALL(glUniformMatrix3fv(loc, uniforms[i].count, GL_FALSE, uniforms[i].value.pf)); break;
		case AGLAT_Mat4: AGL__CALL(glUniformMatrix4fv(loc, uniforms[i].count, GL_FALSE, uniforms[i].value.pf)); break;
		case AGLAT_Int: AGL__CALL(glUniform1iv(loc, uniforms[i].count, uniforms[i].value.pi)); break;
		case AGLAT_IVec2: AGL__CALL(glUniform2iv(loc, uniforms[i].count, uniforms[i].value.pi)); break;
		case AGLAT_IVec3: AGL__CALL(glUniform3iv(loc, uniforms[i].count, uniforms[i].value.pi)); break;
		case AGLAT_IVec4: AGL__CALL(glUniform4iv(loc, uniforms[i].count, uniforms[i].value.pi)); break;
		case AGLAT_Texture:
			a__GLTextureBind(uniforms[i].value.texture, texture_unit);
			AGL__CALL(glUniform1i(loc, texture_unit));
			++texture_unit;
		}
		ATTO_GL_PROFILE_END_NAME("per uniform")
	}
	ATTO_GL_PROFILE_END
}

static void a__GLTargetBind(const AGLDrawTarget *target) {
	ATTO_GL_PROFILE_PREAMBLE
	a__GLFramebufferBind(target->framebuffer);

	if (target->viewport.x != a__gl_state.viewport.x || target->viewport.y != a__gl_state.viewport.y ||
		target->viewport.w != a__gl_state.viewport.w || target->viewport.h != a__gl_state.viewport.h) {
		a__gl_state.viewport.x = target->viewport.x;
		a__gl_state.viewport.y = target->viewport.y;
		a__gl_state.viewport.w = target->viewport.w;
		a__gl_state.viewport.h = target->viewport.h;

		AGL__CALL(glViewport(target->viewport.x, target->viewport.y, target->viewport.w, target->viewport.h));
	}
	ATTO_GL_PROFILE_FUNC(__FUNCTION__, aAppTime() - start);
}

static void a__GLTextureBind(const AGLTexture *texture, GLint unit) {
	ATTO_GL_PROFILE_START
	AGL__CALL(glActiveTexture(GL_TEXTURE0 + unit));
	AGL__CALL(glBindTexture(GL_TEXTURE_2D, texture->_.name));

	AGLTexture *mutable_texture = (AGLTexture *)texture;
	if (mutable_texture->_.min_filter != (GLenum)mutable_texture->min_filter) {
		AGL__CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mutable_texture->min_filter));
		mutable_texture->_.min_filter = (GLenum)mutable_texture->min_filter;
	}
	if (mutable_texture->_.mag_filter != (GLenum)mutable_texture->mag_filter) {
		AGL__CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mutable_texture->mag_filter));
		mutable_texture->_.mag_filter = (GLenum)mutable_texture->mag_filter;
	}
	if (mutable_texture->_.wrap_s != (GLenum)mutable_texture->wrap_s) {
		AGL__CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mutable_texture->wrap_s));
		mutable_texture->_.wrap_s = (GLenum)mutable_texture->wrap_s;
	}
	if (mutable_texture->_.wrap_t != (GLenum)mutable_texture->wrap_t) {
		AGL__CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mutable_texture->wrap_t));
		mutable_texture->_.wrap_t = (GLenum)mutable_texture->wrap_t;
	}
	ATTO_GL_PROFILE_END
}

static void a__GLAttribsBind(const AGLAttribute *attribs, int nattribs) {
	ATTO_GL_PROFILE_START
	int i;
	++a__gl_state.attribs_serial;
	for (i = 0; i < nattribs; ++i) {
		const AGLAttribute *a = attribs + i;
		const GLint loc = a->_.location;
		GLint buffer = a->buffer ? a->buffer->name : 0;
		if (loc < 0) { /*AGL_PRINTFLN("Skipping %s", a->name);*/
			continue;
		}
		if (loc >= ATTO_GL_MAX_ATTRIBS) {
			ATTO_ASSERT("Attrib location is too large");
		}
		if (a__gl_state.attribs[loc].buffer < 0)
			AGL__CALL(glEnableVertexAttribArray(loc));
		if (a__gl_state.attribs[loc].buffer != buffer) {
			AGL__CALL(glBindBuffer(GL_ARRAY_BUFFER, buffer));
			a__gl_state.attribs[loc].buffer = buffer;
		}

		a__gl_state.attribs[loc].serial = a__gl_state.attribs_serial;
		if (a__gl_state.attribs[loc].attrib.size != a->size || a__gl_state.attribs[loc].attrib.type != a->type ||
			a__gl_state.attribs[loc].attrib.normalized != a->normalized ||
			a__gl_state.attribs[loc].attrib.stride != a->stride || a__gl_state.attribs[loc].attrib.ptr != a->ptr) {
			AGL__CALL(glVertexAttribPointer(loc, a->size, a->type, a->normalized, a->stride, a->ptr));
			a__gl_state.attribs[loc].attrib = *a;
		}
	}

	for (i = 0; i < ATTO_GL_MAX_ATTRIBS; ++i) {
		if (a__gl_state.attribs[i].buffer >= 0 && a__gl_state.attribs[i].serial != a__gl_state.attribs_serial) {
			AGL__CALL(glDisableVertexAttribArray(i));
			a__gl_state.attribs[i].buffer = -1;
		}
	}
	ATTO_GL_PROFILE_END
}

static void a__GLDepthBind(AGLDepthParams depth) {
	AGLDepthParams *cur = &a__gl_state.depth;
	if (depth.mode != cur->mode) {
		if (depth.mode == AGLDM_Disabled) {
			AGL__CALL(glDisable(GL_DEPTH_TEST));
		} else {
			AGL__CALL(glEnable(GL_DEPTH_TEST));
			AGL__CALL(glDepthMask(depth.mode == AGLDM_TestAndWrite));
		}
		cur->mode = depth.mode;
	}

	if (cur->mode == AGLDM_Disabled)
		return;

	if (depth.func != cur->func)
		AGL__CALL(glDepthFunc(cur->func = depth.func));
}

static void a__GLBlendBind(const AGLBlendParams *blend) {
	AGLBlendParams *cur = &a__gl_state.blend;
	if (blend->enable != cur->enable) {
		cur->enable = blend->enable;
		if (!blend->enable)
			AGL__CALL(glDisable(GL_BLEND));
		else
			AGL__CALL(glEnable(GL_BLEND));
	}

	if (!cur->enable)
		return;

	if (blend->color.r != cur->color.r || blend->color.g != cur->color.g || blend->color.b != cur->color.b ||
		blend->color.a != cur->color.a) {
		cur->color = blend->color;
		AGL__CALL(glBlendColor(cur->color.r, cur->color.g, cur->color.b, cur->color.a));
	}

	if (blend->equation.rgb != cur->equation.rgb || blend->equation.a != cur->equation.a) {
		cur->equation = blend->equation;
		if (cur->equation.rgb == cur->equation.a)
			AGL__CALL(glBlendEquation(cur->equation.rgb));
		else
			AGL__CALL(glBlendEquationSeparate(cur->equation.rgb, cur->equation.a));
	}

	if (blend->func.src_rgb != cur->func.src_rgb || blend->func.dst_rgb != cur->func.dst_rgb ||
		blend->func.src_a != cur->func.src_a || blend->func.dst_a != cur->func.dst_a) {
		cur->func = blend->func;
		if (cur->func.src_rgb == cur->func.src_a && cur->func.dst_rgb == cur->func.dst_a)
			AGL__CALL(glBlendFunc(cur->func.src_rgb, cur->func.dst_rgb));
		else
			AGL__CALL(glBlendFuncSeparate(cur->func.src_rgb, cur->func.dst_rgb, cur->func.src_a, cur->func.dst_a));
	}
}

static void a__GLCullingBind(AGLCullMode cull, AGLFrontFace front) {
	if (cull != a__gl_state.cull_mode) {
		if (cull == AGLCM_Disable) {
			AGL__CALL(glDisable(GL_CULL_FACE));
			return;
		}

		if (a__gl_state.cull_mode == AGLCM_Disable) {
			AGL__CALL(glEnable(GL_CULL_FACE));
			return;
		}

		AGL__CALL(glCullFace(a__gl_state.cull_mode = cull));
	}

	if (front != a__gl_state.front_face)
		AGL__CALL(glFrontFace(a__gl_state.front_face = front));
}

static void a__GLFramebufferBind(const AGLFramebuffer *fbo) {
	const GLuint desired_binding = fbo ? fbo->name : 0;
	if (a__gl_state.framebuffer.binding != desired_binding)
		AGL__CALL(glBindFramebuffer(GL_FRAMEBUFFER, a__gl_state.framebuffer.binding = desired_binding));
}

AGLFramebuffer aGLFramebufferCreate(AGLFramebufferCreate params) {
	AGLFramebuffer fbo = {0};
	AGL__CALL(glGenFramebuffers(1, &fbo.name));
	AGL__CALL(glBindFramebuffer(GL_FRAMEBUFFER, fbo.name));

	ATTO_ASSERT(params.color);

	if (params.color)
		AGL__CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, params.color->_.name, 0));

	if (params.depth.enable) {
		if (params.depth.texture) {
			AGL__CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, params.depth.texture->_.name, 0));
		} else {
#ifdef ATTO_GLES
			const GLenum depth_component = GL_DEPTH_COMPONENT16;
#else
			const GLenum depth_component = GL_DEPTH_COMPONENT;
#endif
			AGL__CALL(glGenRenderbuffers(1, &fbo.depth_renderbuffer));
			AGL__CALL(glBindRenderbuffer(GL_RENDERBUFFER, fbo.depth_renderbuffer));
			AGL__CALL(glRenderbufferStorage(GL_RENDERBUFFER, depth_component, params.color->width, params.color->height));
			AGL__CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fbo.depth_renderbuffer));
			AGL__CALL(glBindRenderbuffer(GL_RENDERBUFFER, 0));
		}
	} else {
		// Depth disabled
		AGL__CALL(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0));
	}

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	ATTO_ASSERT(status == GL_FRAMEBUFFER_COMPLETE);

	AGL__CALL(glBindFramebuffer(GL_FRAMEBUFFER, a__gl_state.framebuffer.binding));
	return fbo;
}

void aGLFramebufferDestroy(AGLFramebuffer fbo) {
	if (fbo.name) {
		if (fbo.name == a__gl_state.framebuffer.binding)
			AGL__CALL(glBindFramebuffer(GL_FRAMEBUFFER, a__gl_state.framebuffer.binding = 0));
		glDeleteFramebuffers(1, &fbo.name);
	}

	if (fbo.depth_renderbuffer)
		glDeleteRenderbuffers(1, &fbo.depth_renderbuffer);
}

#if 0
void aGLInvalidate(const AGLFramebuffer *fbo) {
	a__GLFramebufferBind(fbo);
	if (!fbo || fbo->name == 0) {
		GLenum attachments[] = {GL_COLOR, GL_DEPTH};
		glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, attachments);
		return;
	}

	ATTO_ASSERT(!"Not implemented");
}
#endif

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* ATTO_GL_H_IMPLEMENT */
