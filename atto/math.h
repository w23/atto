#ifndef ATTO_MATH_DECLARED
#define ATTO_MATH_DECLARED

#include <math.h>
#include <stdint.h>

struct AVec2f { float x, y; };
struct AVec3f { float x, y, z; };
struct AVec4f { float x, y, z, w; };

/* Column-major order */
struct AMat3f { struct AVec3f X, Y, Z; };
struct AMat4f { struct AVec4f X, Y, Z, W; };

/* Scalar helpers */

#define ATTO_MATH_MAKE_MAX(type) \
	static inline type type##Max(type a, type b) { return (a > b) ? a : b; }
ATTO_MATH_MAKE_MAX(float)

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
	const struct AVec2f r = {x, y}; return r;
}

static inline struct AVec2f aVec2fNeg(struct AVec2f a) {
	return aVec2f(-a.x, -a.y);
}

static inline struct AVec2f aVec2fMulf(struct AVec2f v, float f) {
	return aVec2f(v.x * f, v.y * f);
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

static inline float aVec2fLength(struct AVec2f v) {
	return sqrtf(aVec2fDot(v, v));
}

static inline struct AVec2f aVec2fMix(struct AVec2f a, struct AVec2f b, float t) {
	return aVec2fAdd(a, aVec2fMulf(aVec2fSub(b, a), t));
}

/* Vector 3 */

static inline struct AVec3f aVec3f(float x, float y, float z) {
	const struct AVec3f r = {x, y, z}; return r;
}

static inline struct AVec3f aVec3ff(float f) {
	const struct AVec3f r = {f, f, f}; return r;
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

static inline float aVec3fLength(struct AVec3f v) {
	return sqrtf(aVec3fDot(v,v));
}

static inline struct AVec3f aVec3fNormalize(struct AVec3f a) {
	return aVec3fMulf(a, aRevSqrt(aVec3fDot(a,a)));
}

static inline float aVec3fLength2(struct AVec3f v) { return aVec3fDot(v,v); }

static inline struct AVec3f aVec3fMix(struct AVec3f a, struct AVec3f b, float t) {
	return aVec3fAdd(a, aVec3fMulf(aVec3fSub(b, a), t));
}

/* Vector 4 */

static inline struct AVec4f aVec4f(float x, float y, float z, float w) {
	const struct AVec4f r = {x, y, z, w}; return r;
}

static inline struct AVec4f aVec4f3(struct AVec3f v, float w) {
	const struct AVec4f r = {v.x, v.y, v.z, w}; return r;
}

static inline struct AVec4f aVec4fNeg(struct AVec4f v) {
	return aVec4f(-v.x, -v.y, -v.z, -v.w);
}

static inline struct AVec4f aVec4fMulf(struct AVec4f v, float f) {
	v.x *= f; v.y *= f; v.z *= f; v.w *= f; return v;
}

static inline struct AVec4f aVec4fAdd(struct AVec4f a, struct AVec4f b) {
	return aVec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

static inline float aVec4fDot(struct AVec4f a, struct AVec4f b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

/* Matrix 3x3 */
/* Intended usage: orthonormal basis manupulation */

static inline struct AMat3f aMat3fv(
		struct AVec3f x, struct AVec3f y, struct AVec3f z) {
	const struct AMat3f m = {x, y, z}; return m;
}

static inline float aMat3fTrace(struct AMat3f m) {
	return m.X.x + m.Y.y + m.Z.z;
}

static inline struct AMat3f aMat3fScale(float s) {
	const struct AMat3f m = aMat3fv(
			aVec3f(s, 0, 0), aVec3f(0, s, 0), aVec3f(0, 0, s));
	return m;
}

static inline struct AMat3f aMat3fIdentity() { return aMat3fScale(1.f); }

static inline struct AMat3f aMat3fTranspose(struct AMat3f m) {
	return aMat3fv(
			aVec3f(m.X.x, m.Y.x, m.Z.x),
			aVec3f(m.X.y, m.Y.y, m.Z.y),
			aVec3f(m.X.z, m.Y.z, m.Z.z));
}

static inline struct AMat3f aMat3fRotateX(float a) {
	const float c = cosf(a), s = sinf(a);
	return aMat3fv(aVec3f(1, 0, 0), aVec3f(0, c, s), aVec3f(0, -s, c));
}

static inline struct AMat3f aMat3fRotateY(float a) {
	const float c = cosf(a), s = sinf(a);
	return aMat3fv(aVec3f(c, 0, -s), aVec3f(0, 1, 0), aVec3f(s, 0, c));
}

static inline struct AMat3f aMat3fRotateZ(float a) {
	const float c = cosf(a), s = sinf(a);
	return aMat3fv(aVec3f(c, s, 0), aVec3f(-s, c, 0), aVec3f(0, 0, 1));
}

static inline struct AMat3f aMat3fRotateAxis(struct AVec3f r, float a) {
	const float c = cosf(a), c1 = 1 - c, s = sinf(a);
	return aMat3fv(
			aVec3f(c + c1*r.x*r.x, c1*r.x*r.y + r.z*s, c1*r.x*r.z - r.y*s),
			aVec3f(c1*r.x*r.y - r.z*s, c + c1*r.y*r.y, c1*r.y*r.z + r.x*s),
			aVec3f(c1*r.x*r.z + r.y*s, c1*r.y*r.z - r.x*s, c + c1*r.z*r.z));
}

static inline struct AMat3f aMat3fMul(struct AMat3f a, struct AMat3f b) {
	const float *cols = &b.X.x, *rows = &a.X.x;
	static struct AMat3f m;
	float *result = &m.X.x;
	for (int row = 0; row < 3; ++row)
		for (int col = 0; col < 3; ++col) {
			result[row + col*3] =
				cols[col*3 + 0] * rows[row + 0] +
				cols[col*3 + 1] * rows[row + 3] +
				cols[col*3 + 2] * rows[row + 6];
		}
	return m;
}

/* Matrix 4 */
/* Intended usage: homogenous coordinates */

static inline struct AMat4f aMat4fIdentity() {
	const struct AMat4f m = {
			aVec4f(1, 0, 0, 0),
			aVec4f(0, 1, 0, 0),
			aVec4f(0, 0, 1, 0),
			aVec4f(0, 0, 0, 1)};
	return m;
}

static inline struct AMat4f aMat4fPerspective(float n, float f, float w, float h) {
	const struct AMat4f m = {
			aVec4f(2.f*n / w, 0, 0, 0),
			aVec4f(0, 2.f*n / h, 0, 0),
			aVec4f(0, 0, (f+n)/(n-f), -1.f),
			aVec4f(0, 0, (2.f*f*n)/(n-f), 0)};
	return m;
}

static inline struct AMat4f aMat4fTranslation(struct AVec3f t) {
	const struct AMat4f m = {
			aVec4f(1, 0, 0, 0),
			aVec4f(0, 1, 0, 0),
			aVec4f(0, 0, 1, 0),
			aVec4f3(t, 1)};
	return m;
}

static inline struct AMat4f aMat4f3(struct AMat3f o, struct AVec3f t) {
	const struct AMat4f m = {
			aVec4f3(o.X, 0),
			aVec4f3(o.Y, 0),
			aVec4f3(o.Z, 0),
			aVec4f3(t, 1)};
	return m;
}

static inline struct AMat4f aMat4fMul(struct AMat4f a, struct AMat4f b) {
	const float *cols = &b.X.x, *rows = &a.X.x;
	static struct AMat4f m;
	float *result = &m.X.x;
	for (int row = 0; row < 4; ++row)
		for (int col = 0; col < 4; ++col) {
			result[row + col*4] =
				cols[col*4 + 0] * rows[row + 0] +
				cols[col*4 + 1] * rows[row + 4] +
				cols[col*4 + 2] * rows[row + 8] +
				cols[col*4 + 3] * rows[row + 12];
		}
	return m;
}

/* Quaternions (unit) */

struct AQuat { struct AVec3f v; float w; };

static inline struct AQuat aQuatIdentity() {
	const struct AQuat q = {{0, 0, 0}, 1}; return q;
}

static inline struct AQuat aQuatRotation(struct AVec3f axis, float angle) {
	const float c2 = cosf(angle * .5f), s2 = sinf(angle * .5f);
	const struct AQuat q = {aVec3fMulf(axis, s2), c2};
	return q;
}

static inline struct AQuat aQuatMul(struct AQuat a, struct AQuat b) {
	const struct AQuat q = {
		aVec3fAdd(aVec3fAdd(aVec3fCross(a.v, b.v),
					aVec3fMulf(a.v, b.w)), aVec3fMulf(b.v, a.w)),
		a.w * b.w - aVec3fDot(a.v, b.v)};
	return q;
}

static inline float aQuatNorm2(struct AQuat q) {
	return q.w * q.w + aVec3fDot(q.v, q.v);
}

static inline struct AQuat aQuatNormalize(struct AQuat q) {
	const float factor = aRevSqrt(aQuatNorm2(q));
	const struct AQuat r = {aVec3fMulf(q.v, factor), q.w * factor};
	return r;
}

static inline struct AQuat aQuatConjugate(struct AQuat q) {
	const struct AQuat r = {aVec3fNeg(q.v), q.w}; return r;
}

static inline struct AVec3f aQuatX(struct AQuat q) {
	return aVec3f(
			1.f - 2.f * (q.v.y * q.v.y + q.v.z * q.v.z),
			2.f * (q.v.x * q.v.y + q.w * q.v.z),
			2.f * (q.v.x * q.v.z - q.w * q.v.y));
}

static inline struct AVec3f aQuatY(struct AQuat q) {
	return aVec3f(
			2.f * (q.v.x * q.v.y - q.w * q.v.z),
			1.f - 2.f * (q.v.x * q.v.x + q.v.z * q.v.z),
			2.f * (q.v.y * q.v.z + q.w * q.v.x));
}

static inline struct AVec3f aQuatZ(struct AQuat q) {
	return aVec3f(
			2.f * (q.v.x * q.v.z + q.w * q.v.y),
			2.f * (q.v.y * q.v.z - q.w * q.v.x),
			1.f - 2.f * (q.v.x * q.v.x + q.v.y * q.v.y));
}

static inline struct AMat3f aMat3fQuat(struct AQuat q) {
	return aMat3fv(aQuatX(q), aQuatY(q), aQuatZ(q));
}

static inline struct AQuat aQuatMat(struct AMat3f m) {
	struct AQuat q;
	const float tr = aMat3fTrace(m);
	if (tr > 0) { /* will divide by at least .5, which means w is fine */
		q.w = sqrtf(tr + 1.f) * .5f;
		const float r4w = .25f / q.w;
		q.v.x = (m.Y.z - m.Z.y) * r4w;
		q.v.y = (m.Z.x - m.X.z) * r4w;
		q.v.z = (m.X.y - m.Y.x) * r4w;
	} else if (m.X.x > m.Y.y && m.X.x > m.Z.z) { /* x is the largest */
		q.v.x = sqrtf(m.X.x - m.Y.y - m.Z.z + 1.f) * .5f;
		const float r4x = .25f / q.v.x;
		q.v.y = (m.Y.x + m.X.y) * r4x;
		q.v.z = (m.Z.x + m.X.z) * r4x;
		q.w   = (m.Y.z - m.Z.y) * r4x;
	} else if (m.Y.y > m.Z.z) { /* y is the largest */
		q.v.y = sqrtf(- m.X.x + m.Y.y - m.Z.z + 1.f) * .5f;
		const float r4y = .25f / q.v.y;
		q.v.x = (m.Y.x + m.X.y) * r4y;
		q.v.z = (m.Z.y + m.Y.z) * r4y;
		q.w   = (m.Z.x - m.X.z) * r4y;
	} else { /* z is the largest */
		q.v.z = sqrtf(- m.X.x - m.Y.y + m.Z.z + 1.f) * .5f;
		const float r4z = .25f / q.v.z;
		q.v.x = (m.Z.x + m.X.z) * r4z;
		q.v.y = (m.Z.y + m.Y.z) * r4z;
		q.w   = (m.X.y - m.Y.x) * r4z;
	}
	return q;
}

/* Frame of reference */

struct AReFrame {
	struct AQuat orient;
	struct AVec3f transl;
};

static inline struct AReFrame aReFrameLookAt(struct AVec3f pos, struct AVec3f at, struct AVec3f up) {
	const struct AVec3f
			z = aVec3fNormalize(aVec3fSub(pos, at)),
			x = aVec3fNormalize(aVec3fCross(up, z)),
			y = aVec3fCross(z, x);
	const struct AMat3f orient = aMat3fv(x, y, z);
	const struct AReFrame f = {aQuatMat(orient), pos};
	return f;
}

static inline struct AMat4f aMat4fReFrame(struct AReFrame f) {
	return aMat4f3(aMat3fQuat(f.orient), f.transl);
}

static inline struct AVec3f aVec3fMulMat(struct AMat3f m, struct AVec3f v) {
	return aVec3f(
			aVec3fDot(v, aVec3f(m.X.x, m.Y.x, m.Z.x)),
			aVec3fDot(v, aVec3f(m.X.y, m.Y.y, m.Z.y)),
			aVec3fDot(v, aVec3f(m.X.z, m.Y.z, m.Z.z)));
}

static inline struct AReFrame aReFrameInverse(struct AReFrame f) {
	f.orient = aQuatConjugate(f.orient);
	const struct AMat3f m = aMat3fQuat(f.orient);
	f.transl = aVec3fMulMat(m, aVec3fNeg(f.transl));
	return f;
}

static inline struct AMat4f aMat4fLookAt(struct AVec3f pos, struct AVec3f at, struct AVec3f up) {
	const struct AVec3f
			z = aVec3fNormalize(aVec3fSub(pos, at)),
			x = aVec3fNormalize(aVec3fCross(up, z)),
			y = aVec3fCross(z, x);

	return aMat4fMul(
			aMat4f3(aMat3fTranspose(aMat3fv(x, y, z)), aVec3ff(0)),
			aMat4fTranslation(aVec3fNeg(pos)));
}

#endif /* ifndef ATTO_MATH_DECLARED */
