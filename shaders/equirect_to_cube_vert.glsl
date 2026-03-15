#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 local_pos;

uniform mat4 view;
uniform mat4 proj;

void main() {
    local_pos = aPos;
    gl_Position = proj * view * vec4(local_pos, 1.0);
}