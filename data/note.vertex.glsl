#version 330 core
layout (location = 0) in vec2 i_pos;

uniform mat4 u_mat;

void main() {
	gl_Position = u_mat * vec4(i_pos.xy, 0.0, 1.0);
}