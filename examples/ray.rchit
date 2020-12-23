#version 460
#extension GL_GOOGLE_include_directive : enable
#include "ray.glsl"

//hitAttributeEXT RayResult hres;
layout(location = 0) rayPayloadInEXT RayResult rres;

void main() {
	rres.l = gl_HitTEXT;
	rres.p = gl_HitTEXT * gl_WorldRayDirectionEXT + gl_WorldRayOriginEXT;
	rres.n = normalize(rres.p);
}
