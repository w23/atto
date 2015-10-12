#include <math.h>
#include <atto/app.h>

#define ATTO_GL_H_IMPLEMENT
#include <atto/gl.h>

static void init(void);
static void resize(void);
static void paint(ATimeMs);

void atto_app_event(const AEvent *event) {
	switch(event->type) {
		case AET_Init:
			aGLInit();
			init();
			break;

		case AET_Resize:
			resize();
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
		"float r = 10. * sin(uf_time + length(vv2_pos)*6.);"
		"gl_FragColor = abs(vec4("
			"sin(a+uf_time*4.+r),"
			"sin(2.+a+uf_time*3.+r),"
			"sin(4.+a+uf_time*5.+r),"
			"1.));"
	"}"
;

static const char shader_fragment_show[] =
	"uniform sampler2D us2_texture;"
	"uniform float uf_time;"
	"varying vec2 vv2_pos;"
	"void main() {"
		"float a = 5. * atan(vv2_pos.x, vv2_pos.y);"
		"float r = .1 * sin(.4 * uf_time + length(vv2_pos)*3.);"
		"gl_FragColor = "
			"texture2D(us2_texture, vv2_pos*.5 - vec2(.5) + r*vec2(sin(a),cos(a)));"
	"}"
;

static const float vertexes[] = {
	1.f, -1.f,
	0.f, 1.f,
	-1.f, -1.f
};

static const float screenquad[] = {
	1.f, -1.f,
	1.f, 1.f,
	-1.f, -1.f
	-1.f, 1.f,
};

static struct {
	AGLTexture framebuffer;
	AGLFramebufferParams fbp;
	AGLAttribute attr[1];
	AGLProgramUniform uni[2];
	AGLDrawParams draw, show;
	AGLAttribute shattr[1];
} g;

static void init(void) {
	g.draw.proc.program = aGLProgramCreateSimple(shader_vertex, shader_fragment);
	if (g.draw.proc.program <= 0) {
		aAppDebugPrintf("shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}

	g.show.proc.program = aGLProgramCreateSimple(shader_vertex, shader_fragment_show);
	if (g.draw.proc.program <= 0) {
		aAppDebugPrintf("shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}

	g.framebuffer = aGLTextureCreate();

	aGLDrawParamsSetDefaults(&g.draw);
	aGLDrawParamsSetDefaults(&g.show);

	g.attr[0].name = "av2_pos";
	g.attr[0].buffer = 0;
	g.attr[0].size = 2;
	g.attr[0].type = GL_FLOAT;
	g.attr[0].normalized = GL_FALSE;
	g.attr[0].stride = 0;
	g.attr[0].ptr = vertexes;

	g.uni[0].name = "uf_time";
	g.uni[0].type = AGLAT_Float;
	g.uni[0].count = 1;

	g.draw.src.draw.mode = GL_TRIANGLES;
	g.draw.src.draw.count = 3;
	g.draw.src.draw.first = 0;
	g.draw.src.draw.index_buffer = 0;
	g.draw.src.draw.indices_ptr = 0;
	g.draw.src.draw.index_type = 0;
	
	g.draw.src.attribs = g.attr;
	g.draw.src.nattribs = sizeof g.attr / sizeof *g.attr;

	g.draw.proc.uniforms = g.uni;
	g.draw.proc.nuniforms = sizeof g.uni / sizeof *g.uni;

	g.draw.dst.framebuffer = &g.fbp;

	g.fbp.depth_mode = AGLDBM_None;
	g.fbp.color = &g.framebuffer;

	g.shattr[0].name = "av2_pos";
	g.shattr[0].buffer = 0;
	g.shattr[0].size = 2;
	g.shattr[0].type = GL_FLOAT;
	g.shattr[0].normalized = GL_FALSE;
	g.shattr[0].stride = 0;
	g.shattr[0].ptr = screenquad;

	g.uni[1].name = "us2_texture";
	g.uni[1].type = AGLAT_Texture;
	g.uni[1].value.texture = &g.framebuffer;
	g.uni[1].count = 1;

	g.show.src.draw.mode = GL_TRIANGLE_STRIP;
	g.show.src.draw.count = 4;
	g.show.src.draw.first = 0;
	g.show.src.draw.index_buffer = 0;
	g.show.src.draw.indices_ptr = 0;
	g.show.src.draw.index_type = 0;
	
	g.show.src.attribs = g.shattr;
	g.show.src.nattribs = sizeof g.shattr / sizeof *g.shattr;

	g.show.proc.uniforms = g.uni;
	g.show.proc.nuniforms = sizeof g.uni / sizeof *g.uni;

	g.show.dst.framebuffer = 0;
}

static void resize(void) {
	aGLTextureUpload(&g.framebuffer, AGLTF_U8_RGBA,
		a_app_state->width, a_app_state->height, 0);

	glViewport(0, 0, a_app_state->width, a_app_state->height);
}

static void paint(ATimeMs timestamp) {
	float t = timestamp * 1e-3f;
/*
	glClearColor(
		sinf(timestamp*.001f),
		sinf(timestamp*.002f),
		sinf(timestamp*.003f),
		1.f);
	glClear(GL_COLOR_BUFFER_BIT);
*/

	g.uni[0].value.pf = &t;
	aGLDraw(&g.draw);

	aGLDraw(&g.show);
}
