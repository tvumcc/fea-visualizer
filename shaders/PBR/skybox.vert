#version 460 core
layout (location = 0) in vec3 va_position;

out vec3 local_pos;

uniform mat4 proj;
uniform mat4 view;

void main() {
    local_pos = va_position;

    mat4 rot_view = mat4(mat3(view)); // remove the translation part of the view matrix
    vec4 clip_pos = proj * rot_view * vec4(local_pos, 1.0);

    gl_Position = clip_pos.xyww;
}