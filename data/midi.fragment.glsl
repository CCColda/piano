#version 330 core

uniform float u_cutoff;

in vec2 p_xy;
out vec4 o_color;

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
	if (p_xy.y > u_cutoff)
		discard;
	else
		o_color = vec4(hsv2rgb(vec3(p_xy.x - 0.3, 1.0, 1.0)), 1.0);
}