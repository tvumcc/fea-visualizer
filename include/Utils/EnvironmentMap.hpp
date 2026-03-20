#pragma once
#include <glm/gtc/matrix_transform.hpp>

#include "Utils/Mesh.hpp"
#include "Utils/Shader.hpp"
#include "Utils/Camera.hpp"

#include <memory>

/**
 * Stores different cubemaps to be used in image based lighting.
 * Also supports the drawing of a skybox around the scene.
 */
class EnvironmentMap {
public:
    static inline std::shared_ptr<Shader> equirect_to_cube_shader = nullptr;
    static inline std::shared_ptr<Shader> skybox_shader = nullptr;
    static inline std::shared_ptr<Shader> irradiance_convolution_shader = nullptr;
    static inline std::shared_ptr<Shader> prefilter_convolution_shader = nullptr;
    static inline std::shared_ptr<Shader> brdf_convolution_shader = nullptr;
    static inline std::shared_ptr<Mesh> cube_mesh = nullptr;
    static inline std::shared_ptr<Mesh> quad_mesh = nullptr;

    EnvironmentMap(const char* hdr_image_path);

    void draw(std::shared_ptr<Camera> camera);
    void use(std::shared_ptr<AbstractShader> shader);
private:
    unsigned int hdr_texture, env_cubemap, irradiance_map, prefilter_map, brdf_texture;
    unsigned int captureFBO, captureRBO;

    int env_map_resolution = 1024;
    int irradiance_map_resolution = 512;
    int prefilter_map_resolution = 1024;
    int brdf_texture_resolution = 512;

    const glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const glm::mat4 capture_views[6] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };

    bool init_hdr_texture(const char* hdr_image_path);
    void init_env_map();
    void init_irradiance_map();
    void init_prefilter_map();
    void init_brdf_texture();
};