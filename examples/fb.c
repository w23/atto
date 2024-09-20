#include <math.h>
#include <atto/app.h>

#define ATTO_GL_H_IMPLEMENT
//#define ATTO_GL_TRACE
#define ATTO_GL_DEBUG
#include <atto/gl.h>

static const char shader_vertex[] =
#ifdef ATTO_GLES
	"precision mediump float;\n"
#endif
	"attribute vec2 av2_pos;\n"
	"varying vec2 vv2_pos;\n"
	"void main() { vv2_pos = av2_pos; gl_Position = vec4(av2_pos, 0., 1.); }\n";

static const char shader_fragment[] =
#ifdef ATTO_GLES
	"precision mediump float;\n"
#endif
	"uniform float uf_time;\n"
	"varying vec2 vv2_pos;\n"
	"void main() {\n"
	"  float a = 5. * atan(vv2_pos.x, vv2_pos.y);\n"
	"  float r = 10. * sin(uf_time + length(vv2_pos)*6.);\n"
	"  gl_FragColor = abs(vec4(\n"
	"    sin(a+uf_time*4.+r),\n"
	"    sin(2.+a+uf_time*3.+r),\n"
	"    sin(4.+a+uf_time*5.+r),\n"
	"    1.));\n"
	"}\n";

static const char shader_fragment_show[] =
#ifdef ATTO_GLES
	"precision mediump float;\n"
#endif
	"uniform sampler2D us2_texture;\n"
	"uniform float uf_time;\n"
	"varying vec2 vv2_pos;\n"
	"void main() {\n"
	//"  float a = 5. * atan(vv2_pos.x, vv2_pos.y);"
	//"  float r = .1 * sin(.4 * uf_time + length(vv2_pos)*3.);"
	"  gl_FragColor = "
	" vec4(vv2_pos, fract(uf_time), 0.) * .5 + "
	//" vec4(fract(uf_time), 0., 0., 0.); "
	//" vec4(fract(uf_time)) + "
	//"    texture2D(us2_texture, vv2_pos*.5 - vec2(.5) + r*vec2(sin(a),cos(a)));"
	//"    texture2D(us2_texture, fract(uf_time) + vv2_pos*.5 - vec2(.5));"
	"    texture2D(us2_texture, vv2_pos*.5 - vec2(.5));"
	"}\n";

// clang-format off
static const float vertexes[] = {
	1.f, -1.f,
	0.f, 1.f,
	-1.f, -1.f
};
static const float screenquad[] = {
	1.f, -1.f,
	1.f, 1.f,
	-1.f, -1.f,
	-1.f, 1.f,
};
// clang-format on

static struct {
	AGLAttribute attr[1];
	AGLProgramUniform uni[1];

	AGLAttribute shattr[1];
	AGLProgramUniform shuni[2];

	AGLDrawSource draw, show;
	AGLDrawMerge merge;
	AGLDrawTarget screen, fb;

	AGLTexture fbtex;
	AGLFramebuffer fbo;
	AGLClearParams clear;
} g;

static void init(void) {
	g.draw.program = aGLProgramCreateSimple(shader_vertex, shader_fragment);
	if (g.draw.program <= 0) {
		aAppDebugPrintf("draw shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}

	g.show.program = aGLProgramCreateSimple(shader_vertex, shader_fragment_show);
	if (g.show.program <= 0) {
		aAppDebugPrintf("show shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}

	g.fbtex = aGLTextureCreate();

	// Framebuffer texture has no mipmaps, therefore mipmap minification is not available
	g.fbtex.min_filter = AGLTmF_Linear;

	g.clear.r = g.clear.g = g.clear.b = g.clear.a = 0;
	g.clear.depth = 1;
	g.clear.bits = AGLCB_Color;

	g.attr[0].name = "av2_pos";
	g.attr[0].buffer = 0;
	g.attr[0].size = 2;
	g.attr[0].type = GL_FLOAT;
	g.attr[0].normalized = GL_FALSE;
	g.attr[0].stride = 0;
	g.attr[0].ptr = vertexes;

	g.draw.primitive.mode = GL_TRIANGLES;
	g.draw.primitive.count = 3;
	g.draw.primitive.first = 0;
	g.draw.primitive.front_face = AGLFF_CounterClockwise;
	g.draw.primitive.cull_mode = AGLCM_Back;
	g.draw.primitive.index.buffer = 0;
	g.draw.primitive.index.data.ptr = 0;
	g.draw.primitive.index.type = 0;

	g.draw.attribs.p = g.attr;
	g.draw.attribs.n = sizeof g.attr / sizeof *g.attr;
	aGLAttributeLocate(g.draw.program, g.attr, g.draw.attribs.n);

	g.uni[0].name = "uf_time";
	g.uni[0].type = AGLAT_Float;
	g.uni[0].count = 1;

	g.draw.uniforms.p = g.uni;
	g.draw.uniforms.n = 1;
	aGLUniformLocate(g.draw.program, g.uni, g.draw.uniforms.n);

	g.shattr[0].name = "av2_pos";
	g.shattr[0].buffer = 0;
	g.shattr[0].size = 2;
	g.shattr[0].type = GL_FLOAT;
	g.shattr[0].normalized = GL_FALSE;
	g.shattr[0].stride = 0;
	g.shattr[0].ptr = screenquad;

	g.show.attribs.p = g.shattr;
	g.show.attribs.n = sizeof g.shattr / sizeof *g.shattr;
	aGLAttributeLocate(g.show.program, g.shattr, g.show.attribs.n);

	g.shuni[0].name = "uf_time";
	g.shuni[0].type = AGLAT_Float;
	g.shuni[0].count = 1;

	g.shuni[1].name = "us2_texture";
	g.shuni[1].type = AGLAT_Texture;
	g.shuni[1].value.texture = &g.fbtex;
	g.shuni[1].count = 1;

	g.show.uniforms.p = g.shuni;
	g.show.uniforms.n = sizeof g.shuni / sizeof *g.shuni;
	aGLUniformLocate(g.show.program, g.shuni, g.show.uniforms.n);

	g.show.primitive.mode = GL_TRIANGLE_STRIP;
	g.show.primitive.count = 4;
	g.show.primitive.first = 0;
	g.show.primitive.front_face = AGLFF_CounterClockwise;
	g.show.primitive.cull_mode = AGLCM_Back;
	g.show.primitive.index.buffer = 0;
	g.show.primitive.index.data.ptr = 0;
	g.show.primitive.index.type = 0;

	g.screen.framebuffer = 0;

	g.merge.blend.enable = 0;
	g.merge.depth.mode = AGLDM_Disabled;

	g.fb.framebuffer = &g.fbo;
}

static void resize(ATimeUs timestamp, unsigned int old_w, unsigned int old_h) {
	(void)(timestamp);
	(void)(old_w);
	(void)(old_h);

	{
		const AGLTextureUploadData data = {
			.format = AGLTF_U8_RGBA,
			.x = 0,
			.y = 0,
			.width = a_app_state->width,
			.height = a_app_state->height,
			.pixels = 0,
		};
		aGLTextureUpload(&g.fbtex, &data);
	}

	aGLFramebufferDestroy(g.fbo);
	g.fbo = aGLFramebufferCreate((AGLFramebufferCreate){
		.color = &g.fbtex,
		.depth = {
			.enable = 1,
			.texture = NULL,
		},
	});
	ATTO_ASSERT(g.fbo.name);

	g.screen.viewport.x = 0;
	g.screen.viewport.y = 0;
	g.screen.viewport.w = a_app_state->width;
	g.screen.viewport.h = a_app_state->height;

	g.fb.viewport.x = 0;
	g.fb.viewport.y = 0;
	g.fb.viewport.w = a_app_state->width;
	g.fb.viewport.h = a_app_state->height;
}

static void paint(ATimeUs timestamp, float dt) {
	const float t = timestamp * 1e-6f;
	(void)(dt);

	g.clear.r = sinf(t * .1f);
	g.clear.g = sinf(t * .2f);
	g.clear.b = sinf(t * .3f);

	aGLClear(&g.clear, &g.fb);

	g.uni[0].value.pf = &t;
	aGLDraw(&g.draw, &g.merge, &g.fb);

	g.shuni[0].value.pf = &t;
	aGLDraw(&g.show, &g.merge, &g.screen);
}

static void keyPress(ATimeUs timestamp, AKey key, int pressed) {
	(void)(timestamp);
	(void)(pressed);
	if (key == AK_Esc)
		aAppTerminate(0);
}

void attoAppInit(struct AAppProctable *proctable) {
	aGLInit();
	init();

	proctable->resize = resize;
	proctable->paint = paint;
	proctable->key = keyPress;
}
