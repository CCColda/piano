#version 330 core
layout (location = 0) in vec2 i_pos;

uniform mat4 u_mat;
uniform float u_offset;
uniform float u_yscale;
uniform float u_cutoff;

out vec2 p_xy;

void main() {
	p_xy = vec2(i_pos.x, (i_pos.y - u_offset) * u_yscale + u_cutoff);
	gl_Position = u_mat * vec4(p_xy, 0.0, 1.0);
}
