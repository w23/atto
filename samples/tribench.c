#include "atto/app.h"
#define ATTO_GL_H_IMPLEMENT
#include "atto/gl.h"
#define ATTO_MATH_H_IMPLEMENT
#include "atto/math.h"

#include <math.h>
#include <stdlib.h> /* malloc */

static void keyPress(ATimeUs timestamp, AKey key, int pressed) {
	(void)(timestamp); (void)(pressed);
	if (key == AK_Esc)
		aAppTerminate(0);
}

static const char shader_vertex[] =
	"uniform mat4 um4_proj, um4_world;\n"
	"attribute vec3 av3_pos, av3_normal, av3_color, av3_tricenter;\n"
	"varying vec3 vv3_color;\n"
	//"uniform vec3 uv3_lightdir;\n"
	"void main() {\n"
		"vec4 pos = vec4(av3_pos, 1.);\n"
		"vv3_color = av3_color;\n"
		"gl_Position = um4_proj * um4_world * pos;\n"
	"}"
;

static const char shader_fragment[] =
	"varying vec3 vv3_color;\n"
	"void main() {\n"
		"gl_FragColor = vec4(vv3_color, 1.);\n"
	"}"
;

const unsigned int triangles = 8192 * 2;

struct TriVertex {
	struct AVec3f pos, tricenter, normal, color;
};

typedef enum {
	VAttrPos, VAttrTriCenter, VAttrNormal, VAttrColor, VAttr_COUNT
} VAttr;
static struct {
	AGLAttribute attr[VAttr_COUNT];
	AGLProgramUniform pun[2];
	AGLDrawSource draw;
	AGLDrawMerge merge;
	AGLDrawTarget target;
	struct AMat4f projection;

	struct TriVertex *vertices;
	unsigned int vertices_count;
} g;

static struct AVec3f randomVector(struct ALCGRand *rng, float scale) {
	return aVec3fMulf(aVec3fSubf(
			aVec3f(aLcgRandf(rng),aLcgRandf(rng),aLcgRandf(rng)), .5f), 2.f * scale);
}

static void generateTriangles(unsigned int triangles) {
	struct ALCGRand rng = { triangles };

	g.vertices_count = triangles * 3;
	g.vertices = malloc(sizeof(*g.vertices) * g.vertices_count);
	for (unsigned int i = 0; i < triangles; ++i) {
		struct TriVertex *v = g.vertices + i * 3;
		const struct AVec3f center = randomVector(&rng, 1.f);
		v[0].tricenter = v[1].tricenter = v[2].tricenter = center;
		v[0].pos = aVec3fAdd(center, randomVector(&rng, .1f));
		v[1].pos = aVec3fAdd(center, randomVector(&rng, .1f));
		v[2].pos = aVec3fAdd(center, randomVector(&rng, .1f));
		v[0].normal = v[1].normal = v[2].normal = aVec3fNormalize(
				aVec3fCross(
					aVec3fSub(v[1].pos, v[0].pos), aVec3fSub(v[2].pos, v[0].pos)));
		v[0].color = v[1].color = v[2].color
			= aVec3f(aLcgRandf(&rng),aLcgRandf(&rng),aLcgRandf(&rng));
	}
}

static void init(void) {
	generateTriangles(triangles);

	g.draw.program = aGLProgramCreateSimple(shader_vertex, shader_fragment);
	if (g.draw.program <= 0) {
		aAppDebugPrintf("shader error: %s", a_gl_error);
		/* \fixme add fatal */
	}

	g.attr[VAttrPos].name = "av3_pos";
	g.attr[VAttrPos].buffer = NULL;
	g.attr[VAttrPos].size = 3;
	g.attr[VAttrPos].type = GL_FLOAT;
	g.attr[VAttrPos].normalized = GL_FALSE;
	g.attr[VAttrPos].stride = sizeof(*g.vertices);
	g.attr[VAttrPos].ptr = &g.vertices[0].pos;

	g.attr[VAttrNormal].name = "av3_normal";
	g.attr[VAttrNormal].buffer = NULL;
	g.attr[VAttrNormal].size = 3;
	g.attr[VAttrNormal].type = GL_FLOAT;
	g.attr[VAttrNormal].normalized = GL_FALSE;
	g.attr[VAttrNormal].stride = sizeof(*g.vertices);
	g.attr[VAttrNormal].ptr = &g.vertices[0].normal;

	g.attr[VAttrTriCenter].name = "av3_tricenter";
	g.attr[VAttrTriCenter].buffer = NULL;
	g.attr[VAttrTriCenter].size = 3;
	g.attr[VAttrTriCenter].type = GL_FLOAT;
	g.attr[VAttrTriCenter].normalized = GL_FALSE;
	g.attr[VAttrTriCenter].stride = sizeof(*g.vertices);
	g.attr[VAttrTriCenter].ptr = &g.vertices[0].tricenter;

	g.attr[VAttrColor].name = "av3_color";
	g.attr[VAttrColor].buffer = NULL;
	g.attr[VAttrColor].size = 3;
	g.attr[VAttrColor].type = GL_FLOAT;
	g.attr[VAttrColor].normalized = GL_FALSE;
	g.attr[VAttrColor].stride = sizeof(*g.vertices);
	g.attr[VAttrColor].ptr = &g.vertices[0].color;

	g.pun[0].name = "um4_proj";
	g.pun[0].type = AGLAT_Mat4;
	g.pun[0].count = 1;
	g.pun[0].value.pf = &g.projection.X.x;

	g.pun[1].name = "um4_world";
	g.pun[1].type = AGLAT_Mat4;
	g.pun[1].count = 1;

	g.draw.primitive.mode = GL_TRIANGLES;
	g.draw.primitive.count = g.vertices_count;
	g.draw.primitive.first = 0;
	g.draw.primitive.index_buffer = NULL;
	g.draw.primitive.indices_ptr = NULL;
	g.draw.primitive.index_type = 0;

	g.draw.attribs.p = g.attr;
	g.draw.attribs.n = sizeof g.attr / sizeof *g.attr;

	g.draw.uniforms.p = g.pun;
	g.draw.uniforms.n = sizeof g.pun / sizeof *g.pun;

	g.merge.blend.enable = 0;
	g.merge.depth.mode = AGLDM_TestAndWrite;
	g.merge.depth.func = AGLDF_Less;
}

static void resize(ATimeUs timestamp, unsigned int old_w, unsigned int old_h) {
	(void)(timestamp); (void)(old_w); (void)(old_h);
	g.target.viewport.x = g.target.viewport.y = 0;
	g.target.viewport.w = a_app_state->width;
	g.target.viewport.h = a_app_state->height;

	g.target.framebuffer = NULL;

	const float dpm = 38e3f; /* ~96 dpi */
	aMat4fPerspective(&g.projection, .1f, 100.f, a_app_state->width / dpm, a_app_state->height / dpm);
}

static void paint(ATimeUs timestamp, float dt) {
	float t = timestamp * 1e-6f;
	AGLClearParams clear;
	(void)(dt);

	clear.a = 1;
	clear.r = .3f*sinf(t*.1f);
	clear.g = .3f*sinf(t*.2f);
	clear.b = .3f*sinf(t*.3f);
	clear.depth = 1;
	clear.bits = AGLCB_Everything;

	aGLClear(&clear, &g.target);

	struct AMat3f rot;
	aMat3fRotateAxis(&rot, aVec3fNormalize(aVec3f(.7*sinf(t*.37), .7, .7)), t);
	struct AMat4f world;
	aMat4f3(&world, &rot);
	aMat4fTranslate(&world, aVec3f(0,0,-10.));
	g.pun[1].value.pf = &world.X.x;

	const unsigned int split = 8192 * 3;
	for (unsigned int i = 0; i < g.vertices_count / split; ++i) {
		g.draw.primitive.indices_ptr = g.vertices + split * i;
		g.draw.primitive.count = split;
		aGLDraw(&g.draw, &g.merge, &g.target);
	}
}

void attoAppInit(struct AAppProctable *proctable) {
	aGLInit();
	init();

	proctable->resize = resize;
	proctable->paint = paint;
	proctable->key = keyPress;
}
