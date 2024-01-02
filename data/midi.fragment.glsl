#version 330 core

uniform float u_cutoff;

in vec2 p_xy;
out vec4 o_color;

void main() {
	if (p_xy.y > u_cutoff)
		discard;
	else
		o_color = vec4(1.0, 0.0, 0.0, 1.0);
}