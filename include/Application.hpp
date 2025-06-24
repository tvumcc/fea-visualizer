#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Mesh.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "GridInterface.hpp"

#include <memory>

struct Settings {
    int viewing_mode = 1;
};

class Application {
public:
    GLFWwindow* window;
    unsigned int window_width = 1100, window_height = 800;
    unsigned int gui_width = 250;
    bool mouse_activated = true;
    Settings settings;

    std::shared_ptr<Camera> camera;
    std::shared_ptr<GridInterface> grid_interface;

    struct Meshes;
    struct Shaders;

    Application();
    ~Application();
    void load();
    void render();
    void run();

    void render_gui();
private:
    void init_opengl_window(unsigned int window_width, unsigned int window_height);
    void set_glfw_callbacks();
    void init_imgui(const char* font_path, int font_size);
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double dx, double dy);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

struct Application::Meshes {
    static inline std::shared_ptr<Mesh> sphere;

    static void load();
};

struct Application::Shaders {
    static inline std::shared_ptr<Shader> default_shader;
    static inline std::shared_ptr<Shader> solid_color;
    static inline std::shared_ptr<Shader> vertex_color;

    static void load();
};