#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glm/gtc/matrix_inverse.hpp>

#include "Application.hpp"

#include <iostream>

Application::Application() {
    init_opengl_window(window_width, window_height);
    init_imgui("assets/NotoSans.ttf", 20);

    Meshes::load();
    Shaders::load();
    load();
}
Application::~Application() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
}

void Application::Meshes::load() {
    sphere = std::make_shared<Mesh>("assets/sphere.obj");
}
void Application::Shaders::load() {
    default_shader = std::make_shared<Shader>("shaders/default_vert.glsl", "shaders/default_frag.glsl"); 
    solid_color = std::make_shared<Shader>("shaders/solid_color_vert.glsl", "shaders/solid_color_frag.glsl"); 
    vertex_color = std::make_shared<Shader>("shaders/vertex_color_vert.glsl", "shaders/vertex_color_frag.glsl"); 
}
void Application::load() {
    camera = std::make_shared<Camera>(); 
    framebuffer_size_callback(window, window_width, window_height);

    grid_interface = std::make_shared<GridInterface>();
    grid_interface->solid_color_shader = Shaders::solid_color;
    grid_interface->vertex_color_shader = Shaders::vertex_color;

    pslg = std::make_shared<PSLG>();
    pslg->shader = Shaders::solid_color;

    surface = std::make_shared<Surface>();
    surface->shader = Shaders::solid_color;
}

void Application::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    Shaders::default_shader->bind();
    Shaders::default_shader->set_mat4x4("view_proj", camera->get_view_projection_matrix());
    Shaders::solid_color->bind();
    Shaders::solid_color->set_mat4x4("view_proj", camera->get_view_projection_matrix());
    Shaders::vertex_color->bind();
    Shaders::vertex_color->set_mat4x4("view_proj", camera->get_view_projection_matrix());

    grid_interface->draw(camera, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
    pslg->draw();
    surface->draw();
}
void Application::run() {
    while (!glfwWindowShouldClose(window)) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        if (gui_visible) mouse_x -= gui_width;

        if (gui_visible) render_gui();

        if (settings.interact_mode == InteractMode::DrawPSLG)
            pslg->set_pending_point(get_mouse_to_grid_plane_point(mouse_x, mouse_y));
        if (ImGui::GetIO().WantCaptureMouse)
            pslg->pending_point.reset();

        render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
    }
}

void Application::render_gui() {
    const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(gui_width, main_viewport->WorkSize.y));
    ImGui::Begin("Finite Element Visualizer", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));

    if (ImGui::Button("Align Top Down"))
        camera->align_to_plane();
    if (ImGui::Button("Draw PSLG")) {
        camera->align_to_plane();
        settings.interact_mode = InteractMode::DrawPSLG;
    }
    if (ImGui::Button("Clear PSLG")) {
        pslg->clear();
    }
    if (ImGui::Button("Triangulate PSLG")) {
        surface->init_from_PSLG(*pslg);
    }

    ImGui::End();
}

void Application::init_opengl_window(unsigned int window_width, unsigned int window_height) {
    this->window_width = window_width;
    this->window_height = window_height;

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(window_width, window_height, "FEA Visualizer", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetWindowUserPointer(window, this);
    set_glfw_callbacks();
}
void Application::set_glfw_callbacks() {
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
}
void Application::init_imgui(const char* font_path, int font_size) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
    io.Fonts->AddFontFromFileTTF(font_path, font_size);
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

glm::vec3 Application::get_mouse_to_grid_plane_point(double x_pos, double y_pos) {
    glm::vec3 nds_ray = glm::vec3((2.0f * x_pos) / (window_width - (gui_visible ? gui_width : 0)) - 1.0f, 1.0f - (2.0f * y_pos) / window_height, 1.0f);
    glm::vec4 clip_ray = glm::vec4(nds_ray.x, nds_ray.y, -1.0f, 1.0f);
    glm::vec4 eye_ray = glm::inverse(camera->get_projection_matrix()) * clip_ray;
    eye_ray = glm::vec4(eye_ray.x, eye_ray.y, -1.0, 0.0f);
    glm::vec3 world_ray = glm::normalize(glm::vec3(glm::inverse(camera->get_view_matrix()) * eye_ray));

    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 origin = camera->get_camera_position();

    float parameter = -glm::dot(normal, origin) / glm::dot(normal, world_ray);
    glm::vec3 intersection = origin + world_ray * parameter;
    return intersection;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);
	app->window_width = width;
	app->window_height = height;

    if (app->gui_visible) {
        glViewport(app->gui_width, 0, width - app->gui_width, height);
        app->camera->set_aspect_ratio((float)(width - app->gui_width) / height);
    } else {
        glViewport(0, 0, width, height);
        app->camera->set_aspect_ratio((float)width / height);
    }
}
void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    Application* app = (Application*)glfwGetWindowUserPointer(window); 

	static float last_x = (float)app->window_width / 2.0;
	static float last_y = (float)app->window_height / 2.0;
	static bool first_mouse = true;

	if (first_mouse) {
		last_x = x;
		last_y = y;	
		first_mouse = false;
	}

	float dx = (float)x - last_x;
	float dy = last_y - (float)y;
	last_x = (float)x;
	last_y = (float)y;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        app->camera->pan(dx, dy);
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        app->camera->rotate(dx, dy);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


}
void scroll_callback(GLFWwindow* window, double dx, double dy) {
    Application* app = (Application*)glfwGetWindowUserPointer(window); 

    app->camera->zoom(dy);
    app->camera->rotate(dx, 0);
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Application* app = (Application*)glfwGetWindowUserPointer(window); 

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		app->gui_visible = !app->gui_visible;
        framebuffer_size_callback(window, app->window_width, app->window_height);
		glfwSetInputMode(window, GLFW_CURSOR, app->gui_visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
        app->camera->set_orbit_position(glm::vec3(0.0f));
	}

    
    switch (app->settings.interact_mode) {
        case InteractMode::DrawPSLG: {
            if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
                app->pslg->finalize();
                if (!(mods & GLFW_MOD_CONTROL)) {
                    app->settings.interact_mode = InteractMode::Idle;
                }
            }
        } break;
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);

    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);
    if (app->gui_visible) x_pos -= app->gui_width;

    switch (app->settings.interact_mode) {
        case InteractMode::DrawPSLG: {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse) {
                app->pslg->add_pending_point();
            }
        } break;
    }
}