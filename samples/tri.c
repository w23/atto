#include <math.h>
#include <atto/app.h>

#define ATTO_GL_H_IMPLEMENT
#include <atto/gl.h>

static void init(void);
static void paint(ATimeMs);

void atto_app_event(const AEvent *event) {
	switch(event->type) {
		case AET_Init:
			aGLInit();
			init();
			break;

		case AET_Resize:
			glViewport(0, 0, a_global_state->width, a_global_state->height);
			break;

		case AET_Paint:
			paint(event->timestamp);
			break;

		case AET_Key:
			if (event->data.key.key == AK_Esc)
				aAppTerminate(0);
			break;

		default: break;
	}
}

static const char shader_vertex[] =
	"attribute vec2 av2_pos;"
	"varying vec2 vv2_pos;"
	"void main() { vv2_pos = av2_pos; gl_Position = vec4(av2_pos, 0., 1.); }"
;

static const char shader_fragment[] =
	"uniform float uf_time;"
	"varying vec2 vv2_pos;"
	"void main() {"
		"float a = 5. * atan(vv2_pos.x, vv2_pos.y);"
		"gl_FragColor = abs(vec4("
			"sin(a+uf_time*4.),"
			"sin(2.+a+uf_time*3.),"
			"sin(4.+a+uf_time*5.),"
			"1.));"
	"}"
;

static const float vertexes[] = {
	1.f, -1.f,
	0.f, 1.f,
	-1.f, -1.f
};

static struct {
	AGLProgram program;
} g;

static void init(void) {
	g.program = aGLProgramCreateSimple(shader_vertex, shader_fragment);
	if (g.program <= 0)
		aAppDebugPrintf("shader error: %s", a_gl_error);
}

static void paint(ATimeMs timestamp) {
	AGLAttribute attr[1];
	AGLProgramUniform pun[1];
	AGLPaintParams app;
	float t = timestamp * 1e-3f;

	glClearColor(
		sinf(timestamp*.001f),
		sinf(timestamp*.002f),
		sinf(timestamp*.003f),
		1.f);
	glClear(GL_COLOR_BUFFER_BIT);

	attr[0].name = "av2_pos";
	attr[0].buffer = 0;
	attr[0].size = 2;
	attr[0].type = GL_FLOAT;
	attr[0].normalized = GL_FALSE;
	attr[0].stride = 0;
	attr[0].ptr = vertexes;
	aGLAttributeBind(attr, 1, &g.program);

	pun[0].name = "uf_time";
	pun[0].type = AGLAttribute_Float;
	pun[0].pvalue = &t;
	pun[0].count = 1;
	aGLProgramUse(g.program, pun, 1);

	app.mode = GL_TRIANGLES;
	app.count = 3;
	app.first = 0;
	app.index_buffer = 0;
	app.indices_ptr = 0;
	app.index_type = 0;
	aGLPaint(app);
}
