/*
    fem_mesh.vert

    This vertex shader extrudes vertex positions along vertex normals based on
    its nodal value. It also adjusts the normal vector and direction of extrusion
    based on the mesh type selected: Open, Closed, or Mirrored.
*/

#version 460 core
layout (location = 0) in vec3 va_position;
layout (location = 1) in vec3 va_normal;
layout (location = 2) in float va_value;
layout (location = 3) in vec3 va_calculated_normal;

out float value;
out vec3 normal;
out vec3 frag_pos;

uniform mat4 model;
uniform mat4 view_proj;
uniform float vertex_extrusion;
uniform float pixel_discard_threshold;
uniform int mesh_type;

void main() {
    value = va_value;

    vec4 extruded_vertex;
    switch (mesh_type) {
        case 0: { // Open
            normal = va_calculated_normal;
            extruded_vertex = model * vec4((vertex_extrusion * (value - pixel_discard_threshold)) * va_normal + va_position, 1.0);
        } break;
        case 1: { // Closed
            normal = -va_normal;
            extruded_vertex = model * vec4(va_position, 1.0);
        } break;
        case 2: { // Mirrored
            normal = reflect(va_calculated_normal, va_normal);
            extruded_vertex = model * vec4((-vertex_extrusion * (value - pixel_discard_threshold)) * va_normal + va_position, 1.0);
        } break;
    }

    frag_pos = vec3(extruded_vertex);
    gl_Position = view_proj * extruded_vertex;
}