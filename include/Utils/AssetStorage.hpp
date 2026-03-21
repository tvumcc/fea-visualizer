#pragma once
#include <GLFW/glfw3.h>

#include "Utils/ResourceManager.hpp"
#include "Utils/Mesh.hpp"
#include "Utils/Shader.hpp"
#include "Utils/ColorMap.hpp"
#include "Utils/EnvironmentMap.hpp"

/**
 * A utility class to manage the loading and storage of assets.
 * This also manages the loading screen that occurs when the application
 * first starts.
 */
class AssetStorage {
public:
    GLFWwindow* window;

    ResourceManager<Mesh> meshes;
    ResourceManager<AbstractShader> shaders;
    ResourceManager<ColorMap> color_maps;
    ResourceManager<EnvironmentMap> environment_maps;

    std::shared_ptr<Mesh> get_mesh(std::string name);
    std::shared_ptr<Shader> get_shader(std::string name);
    std::shared_ptr<ComputeShader> get_compute_shader(std::string name);
    std::shared_ptr<ColorMap> get_color_map(std::string name);
    std::shared_ptr<EnvironmentMap> get_environment_map(std::string name);

    void load_all_assets();
private:
    unsigned int bg_texture;
    float bg_width, bg_height;

    void render_loading_screen(float progress, const char* message);
    void init_background_texture(const char* path);

    void load_meshes();
    void load_shaders();
    void load_color_maps();
    void load_environment_maps();
    void load_environment_map(const char* name, const char* hdr_image_path, float min_progress, float max_progress);
};