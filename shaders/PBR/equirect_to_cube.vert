/*
    equirect_to_cube.vert

    This vertex shader is used for the conversion of an
    equirectangular map to a cube map done in the subsequent fragment shader.
*/

#version 460 core
layout (location = 0) in vec3 va_position;

out vec3 local_pos;

uniform mat4 view;
uniform mat4 proj;

void main() {
    local_pos = va_position;
    gl_Position = proj * view * vec4(local_pos, 1.0);
}