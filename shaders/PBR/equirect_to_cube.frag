/*
    equirect_to_cube.frag

    This fragment shader performs a projection from equirectangular to spherical 
    so that data from an HDR environment map can be stored in a cube map.
*/

#version 460 core

in vec3 local_pos;

out vec4 FragColor;

uniform sampler2D equirectangular_map;

const vec2 inv_arctan = vec2(0.1591, 0.3183);

void main() {
    vec3 view_vec = normalize(local_pos);
    vec2 uv = vec2(atan(view_vec.z, view_vec.x), asin(view_vec.y)) * inv_arctan + 0.5;
    vec3 color = texture(equirectangular_map, uv).rgb;
    FragColor = vec4(color, 1.0);
}