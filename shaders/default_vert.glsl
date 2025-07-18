#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view_proj;

out vec2 TexCoord;

void main() {
	TexCoord = aTexCoord;
    gl_Position = view_proj * model * vec4(aPos, 1.0);
}