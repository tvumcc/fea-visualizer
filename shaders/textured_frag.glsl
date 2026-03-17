#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture_ID;

void main() {
    FragColor = vec4(texture(texture_ID, TexCoord).rgb, 1.0);
    // FragColor = vec4(TexCoord.x, TexCoord.y, 0.0, 1.0);
}