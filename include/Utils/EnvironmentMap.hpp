#pragma once
#include "Utils/Mesh.hpp"
#include "Utils/Shader.hpp"
#include "Utils/Camera.hpp"

#include <memory>

class EnvironmentMap {
public:
    static inline std::shared_ptr<Shader> equirect_to_cube_shader = nullptr;
    static inline std::shared_ptr<Shader> skybox_shader = nullptr;
    static inline std::shared_ptr<Shader> irradiance_convolution_shader = nullptr;
    static inline std::shared_ptr<Mesh> cube_mesh = nullptr;

    EnvironmentMap(const char* hdr_image_path);

    void draw(std::shared_ptr<Camera> camera);
    void use();
private:
    unsigned int hdr_texture, env_cubemap, irradiance_map;
};