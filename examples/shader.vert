#version 450

layout(location=0) in vec2 aPos;
layout(location=1) in vec2 aColorRG;
layout(location=2) in float aColorB;

layout(push_constant) uniform G {
	float t;
};

layout(location = 0) out vec3 fragColor;

#define rm(a) mat2(cos(a),sin(a),-sin(a),cos(a))

void main() {
    gl_Position = vec4(vec2(600./800., 1.) * (rm(t)*aPos), 0.0, 1.0);
		fragColor = vec3(aColorRG, aColorB);
}
