#version 460 core

in vec2 uv;

out vec4 FragColor;

uniform sampler2D texture_ID;

void main() {
    FragColor = vec4(texture(texture_ID, uv).rgb, 1.0);
}