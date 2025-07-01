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
    ColorMaps::load();
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
    fem_mesh = std::make_shared<Shader>("shaders/fem_mesh_vert.glsl", "shaders/fem_mesh_frag.glsl");
}
void Application::ColorMaps::load() {
    viridis = std::make_shared<ColorMap>(
        "Viridis",
        std::array<glm::vec3, 7>({
            glm::vec3(0.274344,0.004462,0.331359),
            glm::vec3(0.108915,1.397291,1.388110),
            glm::vec3(-0.319631,0.243490,0.156419),
            glm::vec3(-4.629188,-5.882803,-19.646115),
            glm::vec3(6.181719,14.388598,57.442181),
            glm::vec3(4.876952,-13.955112,-66.125783),
            glm::vec3(-5.513165,4.709245,26.582180),
        })
    );
}
void Application::load() {
    camera = std::make_shared<Camera>(); 
    framebuffer_size_callback(window, window_width, window_height);

    grid_interface = std::make_shared<GridInterface>();
    grid_interface->solid_color_shader = Shaders::solid_color;
    grid_interface->vertex_color_shader = Shaders::vertex_color;

    pslg = std::make_shared<PSLG>();
    pslg->shader = Shaders::solid_color;
    pslg->sphere_mesh = Meshes::sphere;

    surface = std::make_shared<Surface>();
    surface->shader = Shaders::solid_color;
    surface->fem_mesh_shader = Shaders::fem_mesh;
    surface->sphere_mesh = Meshes::sphere;
    surface->color_map = ColorMaps::viridis;
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
    Shaders::fem_mesh->bind();
    Shaders::fem_mesh->set_mat4x4("view_proj", camera->get_view_projection_matrix());

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
            pslg->set_pending_point(get_mouse_to_grid_plane_point());
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
    if (pslg->closed()) {
        if (ImGui::Button("Add Hole")) {
            settings.interact_mode = InteractMode::AddHole;
        }
        if (ImGui::Button("Triangulate PSLG")) {
            if (!surface->init_from_PSLG(*pslg))
                ImGui::OpenPopup("PSLG Incomplete");
        }
    }
    if (ImGui::Button("Clear Surface")) {
        surface->clear();
    }
    if (!pslg->holes.empty()) {
        if (ImGui::Button("Clear Holes")) {
            pslg->clear_holes();
        }
    }
    if (ImGui::Button("Enable Brush")) {
        settings.interact_mode = InteractMode::Brush;
    }

    if (ImGui::BeginPopupModal("PSLG Incomplete", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("PSLG must be complete and closed to triangulate.");
        if (ImGui::Button("Close"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + gui_width + 5, main_viewport->WorkPos.y + 5));
    switch (settings.interact_mode) {
        case InteractMode::Idle: {
            ImGui::Begin("Mode: Idle", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Text("These controls also work in other modes.");
            ImGui::Bullet(); ImGui::Text("<RMB> and drag to rotate the camera.");
            ImGui::Bullet(); ImGui::Text("<Shift-LMB> and drag to pan.");
            ImGui::Bullet(); ImGui::Text("<Scroll> to zoom in and out.");
            ImGui::Bullet(); ImGui::Text("<O> to reset pan to the origin.");
            ImGui::Bullet(); ImGui::Text("<E> to toggle the GUI.");
            ImGui::End();
        } break;
        case InteractMode::DrawPSLG: {
            ImGui::Begin("Mode: PSLG Drawing", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Bullet(); ImGui::Text("<LMB> points in sequence to draw connected line segments.");
            ImGui::Bullet(); ImGui::Text("<Ctrl-Enter> to finalize a contiguous section of the drawing.");
            ImGui::Bullet(); ImGui::Text("<Enter> to finalize the entire drawing.");
            ImGui::End();
        } break;
        case InteractMode::AddHole: {
            ImGui::Begin("Mode: Add Hole", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Text("<LMB> in a closed region to designate it as a hole.");
            ImGui::End();
        } break;
        case InteractMode::Brush: {
            ImGui::Begin("Mode: Brush", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Text("<LMB> on elements to set the values of their nodes.");
            ImGui::End();
        } break;
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

glm::vec3 Application::get_world_ray_from_mouse() {
    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);
    if (gui_visible) x_pos -= gui_width;

    glm::vec3 nds_ray = glm::vec3((2.0f * x_pos) / (window_width - (gui_visible ? gui_width : 0)) - 1.0f, 1.0f - (2.0f * y_pos) / window_height, 1.0f);
    glm::vec4 clip_ray = glm::vec4(nds_ray.x, nds_ray.y, -1.0f, 1.0f);
    glm::vec4 eye_ray = glm::inverse(camera->get_projection_matrix()) * clip_ray;
    eye_ray = glm::vec4(eye_ray.x, eye_ray.y, -1.0, 0.0f);
    glm::vec3 world_ray = glm::normalize(glm::vec3(glm::inverse(camera->get_view_matrix()) * eye_ray));

    return world_ray;
}

glm::vec3 Application::get_mouse_to_grid_plane_point() {
    glm::vec3 world_ray = get_world_ray_from_mouse();
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
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
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
            if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
                app->pslg->remove_last_unfinalized_point();
            }
        } break;
    }
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);

    switch (app->settings.interact_mode) {
        case InteractMode::DrawPSLG: {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse && !(mods & GLFW_MOD_SHIFT)) {
                app->pslg->add_pending_point();
            }
        } break;
        case InteractMode::AddHole: {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse && !(mods & GLFW_MOD_SHIFT)) {
                app->pslg->add_hole(app->get_mouse_to_grid_plane_point());
                app->settings.interact_mode = InteractMode::DrawPSLG;
            }
        } break;
        case InteractMode::Brush: {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse && !(mods & GLFW_MOD_SHIFT)) {
                app->surface->brush(app->get_world_ray_from_mouse(), app->camera->get_camera_position(), 0.5f);
            }
        } break;

    }
}