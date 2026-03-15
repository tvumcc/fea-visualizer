#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in float aValue;
layout (location = 3) in vec3 aCalculatedNormal;

uniform mat4 model;
uniform mat4 view_proj;
uniform float vertex_extrusion;
uniform float pixel_discard_threshold;
uniform int mesh_type;

out float value;
out vec3 normal;
out vec3 frag_pos;

void main() {
    value = aValue;

    vec4 extruded_vertex;
    switch (mesh_type) {
        case 0: { // Open
            normal = normalize(aCalculatedNormal);
            extruded_vertex = model * vec4((vertex_extrusion * (value - pixel_discard_threshold)) * aNormal + aPos, 1.0);
        } break;
        case 1: { // Closed
            normal = -normalize(aNormal);
            extruded_vertex = model * vec4(aPos, 1.0);
        } break;
        case 2: { // Mirrored
            normal = normalize(reflect(aCalculatedNormal, aNormal));
            extruded_vertex = model * vec4((-vertex_extrusion * (value - pixel_discard_threshold)) * aNormal + aPos, 1.0);
        } break;
    }

    frag_pos = vec3(extruded_vertex);
    gl_Position = view_proj * extruded_vertex;
}