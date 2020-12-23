#version 460
#extension GL_GOOGLE_include_directive : enable
#include "ray.glsl"

//hitAttributeEXT RayResult hres;
layout(location = 0) rayPayloadInEXT RayResult rres;
layout(binding = 2, set = 0) buffer Vertices { vec3 vertices[]; };

void main() {
	rres.l = gl_HitTEXT;
	rres.p = gl_HitTEXT * gl_WorldRayDirectionEXT + gl_WorldRayOriginEXT;

	const uint vi = gl_PrimitiveID * 3;
	vec3 v0 = vertices[vi + 0];
	vec3 v1 = vertices[vi + 1];
	vec3 v2 = vertices[vi + 2];

	rres.n = normalize(cross(v2-v0, v1-v0));
}
