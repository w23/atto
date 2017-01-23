#ifndef ATTO_MATH_DECLARED
#define ATTO_MATH_DECLARED

#include <math.h>
#include <stdint.h>

/* FIXME no ignore retval */

struct AVec2f { float x, y; };
struct AVec3f { float x, y, z; };
struct AVec4f { float x, y, z, w; };

/* Column-major order */
struct AMat3f { struct AVec3f X, Y, Z; };
struct AMat4f { struct AVec4f X, Y, Z, W; };

/* Scalar helpers */

struct ALCGRand { uint64_t state_; };
static inline uint32_t aLcgRandu(struct ALCGRand *r) {
	r->state_ = r->state_ * 6364136223846793005ull + 1442695040888963407ull;
return (uint32_t)((r->state_ >> 16) & 0xfffffffful);
}
static inline float aLcgRandf(struct ALCGRand *r) {
	return (aLcgRandu(r) >> 8) / (float)(0x00fffffful);
}

static inline float aRevSqrt(float f) { return 1.f / sqrtf(f); }

/* Vector 2 */

static inline struct AVec2f aVec2f(float x, float y) {
	struct AVec2f r; r.x = x; r.y = y; return r;
}

static inline struct AVec2f aVec2fNeg(struct AVec2f a) {
	return aVec2f(-a.x, -a.y);
}

static inline struct AVec2f aVec2fAdd(struct AVec2f a, struct AVec2f b) {
	return aVec2f(a.x + b.x, a.y + b.y);
}

static inline struct AVec2f aVec2fSub(struct AVec2f a, struct AVec2f b) {
	return aVec2fAdd(a, aVec2fNeg(b));
}

static inline struct AVec2f aVec2fMul(struct AVec2f a, struct AVec2f b) {
	return aVec2f(a.x * b.x, a.y * b.y);
}

static inline struct AVec2f aVec2fDiv(struct AVec2f a, struct AVec2f b) {
	return aVec2f(a.x / b.x, a.y / b.y);
}

static inline float aVec2fDot(struct AVec2f a, struct AVec2f b) {
	return a.x * b.x + a.y * b.y;
}

/* Vector 3 */

static inline struct AVec3f aVec3f(float x, float y, float z) {
	struct AVec3f r; r.x = x; r.y = y; r.z = z; return r;
}

static inline struct AVec3f aVec3fNeg(struct AVec3f a) {
	return aVec3f(-a.x, -a.y, -a.z);
}

static inline struct AVec3f aVec3fAdd(struct AVec3f a, struct AVec3f b) {
	return aVec3f(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline struct AVec3f aVec3fSub(struct AVec3f a, struct AVec3f b) {
	return aVec3fAdd(a, aVec3fNeg(b));
}

static inline struct AVec3f aVec3fSubf(struct AVec3f a, float b) {
	return aVec3f(a.x - b, a.y - b, a.z - b);
}

static inline struct AVec3f aVec3fMul(struct AVec3f a, struct AVec3f b) {
	return aVec3f(a.x * b.x, a.y * b.y, a.z * b.z);
}

static inline struct AVec3f aVec3fMulf(struct AVec3f a, float b) {
	return aVec3f(a.x * b, a.y * b, a.z * b);
}

static inline float aVec3fDot(struct AVec3f a, struct AVec3f b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline struct AVec3f aVec3fCross(struct AVec3f a, struct AVec3f b) {
	return aVec3f(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
	);
}

static inline struct AVec3f aVec3fNormalize(struct AVec3f a) {
	return aVec3fMulf(a, aRevSqrt(aVec3fDot(a,a)));
}

/* Vector 4 */

static inline struct AVec4f aVec4f(float x, float y, float z, float w) {
	struct AVec4f r; r.x = x; r.y = y; r.z = z; r.w = w; return r;
}

static inline struct AVec4f aVec4f3(struct AVec3f v, float w) {
	struct AVec4f r; r.x = v.x; r.y = v.y; r.z = v.z; r.w = w; return r;
}

static inline struct AVec4f aVec4fAdd(struct AVec4f a, struct AVec4f b) {
	return aVec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

/* Matrix 3x3 */
/* Intended usage: orthonormal basis manupulation */

void aMat3fScale(struct AMat3f *m, float s);
static inline void aMat3fIdentity(struct AMat3f *m) { aMat3fScale(m, 1.f); }
void aMat3fRotateX(struct AMat3f *m, float a);
void aMat3fRotateY(struct AMat3f *m, float a);
void aMat3fRotateZ(struct AMat3f *m, float a);
void aMat3fRotateAxis(struct AMat3f *m, struct AVec3f r, float a);
void aMat3fMul(struct AMat3f *dst, struct AMat3f *a, struct AMat3f *b);

/* Matrix 4 */
/* Intended usage: homogenous coordinates */

void aMat4fIdentity(struct AMat4f *m);
void aMat4fPerspective(struct AMat4f *m, float n, float f, float w, float h);
void aMat4fTranslate(struct AMat4f *m, struct AVec3f t);
void aMat4f3(struct AMat4f *m, struct AMat3f *o);
void aMat4fMul(struct AMat4f *dst, struct AMat4f *a, struct AMat4f *b);

#ifdef ATTO_MATH_H_IMPLEMENT
#ifndef ATTO_MATH_IMPLEMENTED
#define ATTO_MATH_IMPLEMENTED
#endif /* ifndef ATTO_MATH_IMPLEMENTED */
#endif /* ifdef ATTO_MATH_H_IMPLEMENT */

void aMat3fScale(struct AMat3f *m, float s) {
	m->X = aVec3f(s, 0, 0);
	m->Y = aVec3f(0, s, 0);
	m->Z = aVec3f(0, 0, s);
}

void aMat3fRotateX(struct AMat3f *m, float a) {
	const float c = cosf(a), s = sinf(a);
	m->X = aVec3f(1, 0, 0);
	m->Y = aVec3f(0, c, s);
	m->Z = aVec3f(0, -s, c);
}

void aMat3fRotateY(struct AMat3f *m, float a) {
	const float c = cosf(a), s = sinf(a);
	m->X = aVec3f(c, 0, -s);
	m->Y = aVec3f(0, 1, 0);
	m->Z = aVec3f(s, 0, c);
}

void aMat3fRotateZ(struct AMat3f *m, float a) {
	const float c = cosf(a), s = sinf(a);
	m->X = aVec3f(c, s, 0);
	m->Y = aVec3f(-s, c, 0);
	m->Z = aVec3f(0, 0, 1);
}

void aMat3fRotateAxis(struct AMat3f *m, struct AVec3f r, float a) {
	const float c = cosf(a), c1 = 1 - c, s = sinf(a);
	m->X = aVec3f(c + c1*r.x*r.x, c1*r.x*r.y + r.z*s, c1*r.x*r.z - r.y*s);
	m->Y = aVec3f(c1*r.x*r.y - r.z*s, c + c1*r.y*r.y, c1*r.y*r.z + r.x*s);
	m->Z = aVec3f(c1*r.x*r.z + r.y*s, c1*r.y*r.z - r.x*s, c + c1*r.z*r.z);
}

void aMat3fMul(struct AMat3f *dst, struct AMat3f *a, struct AMat3f *b) {
	const float *cols = &a->X.x, *rows = &b->X.x;
	float *result = &dst->X.x;

	for (int row = 0; row < 3; ++row)
		for (int col = 0; col < 3; ++col) {
			result[row + col*3] =
				cols[col*3 + 0] * rows[row + 0] +
				cols[col*3 + 1] * rows[row + 3] +
				cols[col*3 + 2] * rows[row + 6];
		}
}

void aMat4fIdentity(struct AMat4f *m) {
	m->X = aVec4f(1, 0, 0, 0);
	m->Y = aVec4f(0, 1, 0, 0);
	m->Z = aVec4f(0, 0, 1, 0);
	m->W = aVec4f(0, 0, 0, 1);
}

void aMat4fPerspective(struct AMat4f *m, float n, float f, float w, float h) {
	m->X = aVec4f(2.f*n / w, 0, 0, 0);
	m->Y = aVec4f(0, 2.f*n / h, 0, 0);
	m->Z = aVec4f(0, 0, (f+n)/(n-f), -1.f);
	m->W = aVec4f(0, 0, (2.f*f*n)/(n-f), 0);
}

void aMat4fTranslate(struct AMat4f *m, struct AVec3f t) {
	m->W = aVec4fAdd(m->W, aVec4f3(t, 0));
}

void aMat4f3(struct AMat4f *m, struct AMat3f *o) {
	m->X = aVec4f3(o->X, 0);
	m->Y = aVec4f3(o->Y, 0);
	m->Z = aVec4f3(o->Z, 0);
	m->W = aVec4f(0, 0, 0, 1);
}

void aMat4fMul(struct AMat4f *dst, struct AMat4f *a, struct AMat4f *b) {
	const float *cols = &a->X.x, *rows = &b->X.x;
	float *result = &dst->X.x;

	for (int row = 0; row < 4; ++row)
		for (int col = 0; col < 4; ++col) {
			result[row + col*4] =
				cols[col*4 + 0] * rows[row + 0] +
				cols[col*4 + 1] * rows[row + 4] +
				cols[col*4 + 2] * rows[row + 8] +
				cols[col*4 + 3] * rows[row + 12];
		}
}

#endif /* ifndef ATTO_MATH_DECLARED */
