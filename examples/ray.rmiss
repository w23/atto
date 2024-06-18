#version 460
#extension GL_GOOGLE_include_directive : enable
#include "ray.glsl"

layout(location = 0) rayPayloadInEXT RayResult rres;

void main() {
	rres.l = 1e6;
	rres.p = vec3(1e6);
	rres.n = vec3(1., 0., 0.);
}
