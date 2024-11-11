#include "atto/app.h"

#define ATTO_GL_H_IMPLEMENT
#define ATTO_GL_PRINT_LIMITS
#include "atto/gl.h"

#define ATTO_IO_H_IMPLEMENT
#include "atto/io.h"

static void keyPress(ATimeUs timestamp, AKey key, int pressed) {
	(void)(timestamp);
	(void)(pressed);
	if (key == AK_Esc)
		aAppTerminate(0);
}

static const char shader_vertex[] =
#ifdef ATTO_GLES
  "#version 310\n"
	"precision mediump float;\n"
	"out vec2 vv2_pos;\n"
#else
  "#version 130\n"
	"varying vec2 vv2_pos;\n"
#endif
	"void main() {\n"
	// Single full-screen triangle of [(-1, -1), (-1, 2), (2, -1)]
	"  vec2 t = vec2(ivec2((gl_VertexID >> 1)&1, gl_VertexID&1));\n"
	"  vv2_pos = t * 5. - 1.;\n"
	"  gl_Position = vec4(vv2_pos, 0., 1.);\n"
	"}\n";

static const char shader_fragment_header[] =
#ifdef ATTO_GLES
	"precision mediump float;\n"
	"in vec2 vv2_pos;\n"
#else
	"varying vec2 vv2_pos;\n"
#endif
	"uniform float uf_time;\n";

static const char shader_fragment_dummy[] =
	"vec3 fragMain(vec2 uv, float t) {\n"
	"  return vec3(uv, sin(t));\n"
	"}\n";

static const char shader_fragment_footer[] =
	"void main() {\n"
	"  vec2 uv = vv2_pos;\n"
	"  gl_FragColor = vec4(fragMain(uv, uf_time), 1.);\n"
	"}\n";

static struct {
	AGLProgramUniform pun[1];
	AGLDrawSource draw;
	AGLDrawMerge merge;
	AGLDrawTarget target;

	const char *filename;
	struct Aio_monitor_t *monitor;
} g;

static AGLProgram createProgramFromFragmentBody(const char *fragment_body) {
	const char *vert[] = { shader_vertex, NULL };
	const char *frag[] = { shader_fragment_header, fragment_body, shader_fragment_footer, NULL };
	AGLProgram prog = aGLProgramCreate(vert, frag);
	if (prog <= 0) {
		aAppDebugPrintf("shader error: %s", a_gl_error);
		return 0;
	}
	return prog;
}

static AGLProgram createProgramFromFile(const char *filename) {
	char buffer[64*1024];
	const int size = aIoFileReadInto(filename, buffer, sizeof(buffer) - 1);
	if (size <= 0) {
		aAppDebugPrintf("Unable to read shader file \"%s\"", filename);
		return 0;
	}
	return createProgramFromFragmentBody(buffer);
}

static void init(void) {
	g.pun[0].name = "uf_time";
	g.pun[0].type = AGLAT_Float;
	g.pun[0].count = 1;

	g.draw.uniforms.p = g.pun;
	g.draw.uniforms.n = sizeof g.pun / sizeof *g.pun;

	g.draw.program = createProgramFromFile(g.filename);
	if (g.draw.program <= 0) {
		aAppDebugPrintf("failed to create shader program from file \"%s\"", g.filename);
		g.draw.program = createProgramFromFragmentBody(shader_fragment_dummy);
		if (g.draw.program <= 0) {
			aAppDebugPrintf("failed to create placeholder shader program, error: %s", a_gl_error);
			aAppTerminate(1);
		}
	}

	aGLUniformLocate(g.draw.program, g.pun, g.draw.uniforms.n);

	g.draw.primitive.mode = GL_TRIANGLES;
	g.draw.primitive.count = 3;
	g.draw.primitive.first = 0;
	g.draw.primitive.index.buffer = 0;
	g.draw.primitive.index.data.ptr = 0;
	g.draw.primitive.index.type = 0;

	g.draw.attribs.p = NULL;
	g.draw.attribs.n = 0;

	g.merge.blend.enable = 0;
	g.merge.depth.mode = AGLDM_Disabled;
}

static void resize(ATimeUs timestamp, unsigned int old_w, unsigned int old_h) {
	(void)(timestamp);
	(void)(old_w);
	(void)(old_h);
	g.target.viewport.x = g.target.viewport.y = 0;
	g.target.viewport.w = a_app_state->width;
	g.target.viewport.h = a_app_state->height;

	g.target.framebuffer = 0;
}

static void paint(ATimeUs timestamp, float dt) {
	float t = timestamp * 1e-6f;
	(void)(dt);

	if (aIoMonitorCheck(g.monitor)) {
		aAppDebugPrintf("File \"%s\" updated", g.filename);
		AGLProgram new_prog = createProgramFromFile(g.filename);
		if (new_prog > 0) {
			aGLProgramDestroy(g.draw.program);
			g.draw.program = new_prog;
			aGLUniformLocate(g.draw.program, g.pun, g.draw.uniforms.n);
		}
	}

	const AGLClearParams clear = {
		.bits = AGLCB_Everything,
	};
	aGLClear(&clear, &g.target);

	g.pun[0].value.pf = &t;
	aGLDraw(&g.draw, &g.merge, &g.target);
}

void attoAppInit(struct AAppProctable *proctable) {
	if (a_app_state->argc < 2) {
		aAppDebugPrintf("Usage: %s shader.frag", a_app_state->argv[0]);
		aAppTerminate(1);
	}

	g.filename = a_app_state->argv[1];
	g.monitor = aIoMonitorOpen(g.filename);

	aGLInit();
	init();

	proctable->resize = resize;
	proctable->paint = paint;
	proctable->key = keyPress;
}
