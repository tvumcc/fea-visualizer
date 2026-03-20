#version 460 core

in vec3 local_pos;

out vec4 FragColor;
  
uniform samplerCube environment_map;
  
void main() {
    vec3 env_color = textureLod(environment_map, local_pos, 0.0).rgb;
    
    env_color = env_color / (env_color + vec3(1.0));
    env_color = pow(env_color, vec3(1.0/2.2));

    FragColor = vec4(env_color, 1.0);
}