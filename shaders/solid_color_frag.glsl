#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 object_color;

void main() {
    FragColor = vec4(object_color, 1.0);
}