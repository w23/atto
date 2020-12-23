#version 460
#extension GL_GOOGLE_include_directive : enable
#include "ray.glsl"

layout(location = 0) rayPayloadInEXT RayResult rres;
layout(binding = 2, set = 0) buffer Vertices { vec4 vertices[]; };
hitAttributeEXT vec2 bary;

void main() {
	rres.l = gl_HitTEXT;
	rres.p = gl_HitTEXT * gl_WorldRayDirectionEXT + gl_WorldRayOriginEXT;

	rres.num = float(gl_PrimitiveID);
	const uint vi = gl_PrimitiveID * 3;
	vec3 v0 = vertices[vi + 0].xyz;
	vec3 v1 = vertices[vi + 1].xyz;
	vec3 v2 = vertices[vi + 2].xyz;

	rres.n = normalize(cross(v2-v0, v1-v0));
	if (gl_HitKindEXT == gl_HitKindFrontFacingTriangleEXT)
		rres.n = -rres.n;
	//vec3 b = vec3(1. - bary.x - bary.y, bary.x, bary.y);
	//rres.n = v0 * b.x + v1 * b.y + v2 * b.z;
	//rres.n = b;
}
