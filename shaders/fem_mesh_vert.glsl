#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in float aValue;

uniform mat4 model;
uniform mat4 view_proj;

out float value;

void main() {
    value = aValue;
    gl_Position = view_proj * (model * vec4(min(0.7, max(0.0, aValue)) * aNormal + aPos, 1.0));
}