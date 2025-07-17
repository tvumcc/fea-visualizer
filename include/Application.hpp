#pragma once
#include <imgui/imgui.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#include "Mesh.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "GridInterface.hpp"
#include "PSLG.hpp"
#include "Surface.hpp"
#include "ColorMap.hpp"
#include "Solver.hpp"
#include "ResourceManager.hpp"
#include "BVH.hpp"

#include <memory>

enum class InteractMode {
    Idle,
    DrawPSLG,
    AddHole,
    Brush,
};

enum class SolverType {
    Heat = 0,
    Wave,
    Advection_Diffusion
};

struct Settings {
    InteractMode interact_mode = InteractMode::Idle;
    bool draw_grid_interface = true;
    bool draw_surface_wireframe = true;
    bool extrude_nodes = true;
    bool paused = false;
    int bvh_depth = 10;
    float brush_strength = 1.0f;

    std::vector<std::pair<GLuint, ImVec2>> solver_equation_textures;
    std::vector<const char*> solvers = {"Heat", "Wave", "Advection-Diffusion"};
    int selected_solver = (int)SolverType::Heat;

    std::vector<std::pair<GLuint, ImVec2>> color_map_icon_textures;
    std::vector<const char*> color_maps;
    int selected_color_map = 0;

    std::string error_message = "";

    void init_color_maps(ResourceManager<ColorMap> cmaps) {
        cmaps.perform_action_on_all([this](ColorMap& cmap){
            color_maps.push_back(cmap.name.c_str());
        });
    }

    void init_equation_textures() {
        for (int i = 0; i < solvers.size(); i++) {
            GLuint texture;
            int width, height, channels;
            unsigned char *data = stbi_load(std::format("assets/equations/{}_Equation.png", solvers[i]).c_str(), &width, &height, &channels, 0);
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            solver_equation_textures.push_back({texture, ImVec2(width / 3, height / 3)});
        }
    }

    void init_color_map_icon_textures() {
        for (int i = 0; i < color_maps.size(); i++) {
            GLuint texture;
            int width, height, channels;
            unsigned char *data = stbi_load(std::format("assets/cmap_icons/{}.png", color_maps[i]).c_str(), &width, &height, &channels, 0);
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            color_map_icon_textures.push_back({texture, ImVec2(width / 3, height / 3)});
        }
    }
};

class Application {
public:
    GLFWwindow* window;
    unsigned int window_width = 1100, window_height = 800;
    unsigned int gui_width = 250;
    bool gui_visible = true;
    Settings settings;

    std::shared_ptr<Camera> camera;
    std::shared_ptr<GridInterface> grid_interface;
    std::shared_ptr<PSLG> pslg;
    std::shared_ptr<Surface> surface;
    std::shared_ptr<Solver> solver;
    std::shared_ptr<BVH> bvh;

    ResourceManager<Mesh> meshes;
    ResourceManager<Shader> shaders;
    ResourceManager<ColorMap> color_maps;

    Application();
    ~Application();
    void load();
    void load_resources();
    void render();
    void render_gui();
    void run();

    void reset_orbit_position();
    void align_top_down();
    void clear_pslg();
    void clear_holes();
    void clear_solver();
    void delete_surface();
    void init_surface_from_pslg();
    void init_surface_from_obj();
    void switch_solver(SolverType new_solver);
    void switch_color_map(const char* new_color_map);
    void switch_mode(InteractMode mode);

    void brush(glm::vec3 world_ray, glm::vec3 origin, float value);
    glm::vec3 get_world_ray_from_mouse();
    glm::vec3 get_mouse_to_grid_plane_point();
private:
    void init_opengl_window(unsigned int window_width, unsigned int window_height);
    void set_glfw_callbacks();
    void init_imgui(const char* font_path, int font_size);
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double dx, double dy);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);