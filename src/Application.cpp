#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <nfd.h>

#include "Application.hpp"
#include "HeatSolver.hpp"
#include "WaveSolver.hpp"
#include "AdvectionDiffusionSolver.hpp"

#include <iostream>

Application::Application() {
    init_opengl_window(window_width, window_height);
    init_imgui("assets/NotoSans.ttf", 20);

    load_resources();
    load();
}
Application::~Application() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
}

/**
 * Load resources that are shared between multiple classes in the 
 * program such as Shaders, Meshes, and ColorMaps.
 */
void Application::load_resources() {
    // Meshes
    meshes.add("sphere", std::make_shared<Mesh>("assets/sphere.obj"));

    // Shaders
    shaders.add("default", std::make_shared<Shader>("shaders/default_vert.glsl", "shaders/default_frag.glsl"));
    shaders.add("solid_color", std::make_shared<Shader>("shaders/solid_color_vert.glsl", "shaders/solid_color_frag.glsl"));
    shaders.add("vertex_color", std::make_shared<Shader>("shaders/vertex_color_vert.glsl", "shaders/vertex_color_frag.glsl"));
    shaders.add("fem_mesh", std::make_shared<Shader>("shaders/fem_mesh_vert.glsl", "shaders/fem_mesh_frag.glsl"));
    shaders.add("wireframe", std::make_shared<Shader>("shaders/fem_mesh_vert.glsl", "shaders/solid_color_frag.glsl"));

    // Color Maps, make sure the name in the ResourceManager and the ColorMap are the same.
    color_maps.add("Viridis", std::make_shared<ColorMap>(
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
    ));
    color_maps.add("Plasma", std::make_shared<ColorMap>(
        "Plasma",
        std::array<glm::vec3, 7>({
            glm::vec3(0.05873234392399702, 0.02333670892565664, 0.5433401826748754),
            glm::vec3(2.176514634195958, 0.2383834171260182, 0.7539604599784036),
            glm::vec3(-2.689460476458034, -7.455851135738909, 3.110799939717086),
            glm::vec3(6.130348345893603, 42.3461881477227, -28.51885465332158),
            glm::vec3(-11.10743619062271, -82.66631109428045, 60.13984767418263),
            glm::vec3(10.02306557647065, 71.41361770095349, -54.07218655560067),
            glm::vec3(-3.658713842777788, -22.93153465461149, 18.19190778539828),
        })
    ));
    color_maps.add("Spectral_r", std::make_shared<ColorMap>(
        "Spectral_r",
        std::array<glm::vec3, 7>({
            glm::vec3(0.426208,0.275203,0.563277),
            glm::vec3(-5.321958,3.761848,5.477444),
            glm::vec3(42.422339,-15.057685,-57.232349),
            glm::vec3(-100.917716,57.029463,232.590601),
            glm::vec3(106.422535,-116.177338,-437.123306),
            glm::vec3(-48.460514,103.570154,378.807920),
            glm::vec3(6.016269,-33.393152,-122.850806),
        })
    ));
    settings.init_color_maps(color_maps);
    settings.init_equations();
}

/**
 * This function is called before the main loop and is primarily used to
 * initialize pointers to shared resources.
 */
void Application::load() {
    camera = std::make_shared<Camera>(); 
    framebuffer_size_callback(window, window_width, window_height);

    grid_interface = std::make_shared<GridInterface>();
    grid_interface->solid_color_shader = shaders.get("solid_color");
    grid_interface->vertex_color_shader = shaders.get("vertex_color");

    pslg = std::make_shared<PSLG>();
    pslg->shader = shaders.get("solid_color");
    pslg->sphere_mesh = meshes.get("sphere");

    surface = std::make_shared<Surface>();
    surface->wireframe_shader = shaders.get("wireframe");
    surface->fem_mesh_shader = shaders.get("fem_mesh");
    surface->sphere_mesh = meshes.get("sphere");

    switch_solver((SolverType)settings.selected_solver);
    switch_color_map(settings.color_maps[settings.selected_color_map]);
}

/**
 * Renders to the window everything that needs to be drawn.
 */
void Application::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    shaders.perform_action_on_all([this](Shader& shader){
        shader.bind();
        shader.set_mat4x4("view_proj", this->camera->get_view_projection_matrix()); 
    });

    pslg->draw();

    shaders.get("fem_mesh")->bind();
    shaders.get("fem_mesh")->set_bool("extrude_nodes", settings.extrude_nodes);
    shaders.get("wireframe")->bind();
    shaders.get("wireframe")->set_bool("extrude_nodes", settings.extrude_nodes);
    surface->draw(settings.draw_surface_wireframe);

    if (settings.draw_grid_interface) grid_interface->draw(camera, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
}

/**
 * Renders the GUI elements with ImGUI.
 */
void Application::render_gui() {
    const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y));
	ImGui::SetNextWindowSize(ImVec2(gui_width, main_viewport->WorkSize.y));
    ImGui::Begin("Finite Element Visualizer", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 6;
    style.WindowRounding = 6;
    style.FrameBorderSize = 1;

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.17f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.17f, 0.48f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.17f, 0.48f, 0.54f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.46f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.46f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.34f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_Button] = ImVec4(0.46f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.34f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_Header] = ImVec4(0.46f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.26f, 0.98f, 0.40f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.34f, 0.26f, 0.98f, 0.40f);

    ImGui::SeparatorText("Camera");
    if (ImGui::Button("Reset Orbit Position")) reset_orbit_position();
    if (ImGui::Button("Align Top Down"))       align_top_down();
    ImGui::Checkbox("Draw Grid", &settings.draw_grid_interface);

    ImGui::SeparatorText("Surface");
    if (!surface->initialized) {
        if (ImGui::Button("Draw PSLG"))            switch_mode_draw_pslg();
        if (!pslg->empty()) {
            ImGui::Indent();
            if (ImGui::Button("Clear PSLG"))           clear_pslg();
            if (pslg->closed()) {
                if (ImGui::Button("Add Hole"))         switch_mode_add_hole();
                if (!pslg->holes.empty())
                    if (ImGui::Button("Clear Holes"))  clear_holes();
                if (ImGui::Button("Init from PSLG"))   init_surface_from_pslg();
            }
            ImGui::Unindent();
        }
        if (ImGui::Button("Init from .obj"))   init_surface_from_obj();
    } else {
        ImGui::Text(std::format("{} Equation:", settings.solvers[settings.selected_solver]).c_str());
        ImGui::Image(settings.strong_form_equations[settings.selected_solver].first, settings.strong_form_equations[settings.selected_solver].second);
        ImGui::Separator();
        ImGui::Checkbox("Draw Wireframe", &settings.draw_surface_wireframe);
        ImGui::Checkbox("Extrude Nodes", &settings.extrude_nodes);
        if (ImGui::BeginCombo("Solver", settings.solvers[settings.selected_solver], ImGuiComboFlags_WidthFitPreview)) {
            for (int i = 0; i < settings.solvers.size(); i++) {
                const bool is_selected = (settings.selected_solver == i);
                if (ImGui::Selectable(settings.solvers[i], is_selected)) {
                    settings.selected_solver = i;
                    switch_solver((SolverType)i);
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("Color Map", settings.color_maps[settings.selected_color_map], ImGuiComboFlags_WidthFitPreview)) {
            for (int i = 0; i < settings.color_maps.size(); i++) {
                const bool is_selected = (settings.selected_color_map == i);
                if (ImGui::Selectable(settings.color_maps[i], is_selected)) {
                    settings.selected_color_map = i;
                    switch_color_map(settings.color_maps[i]);
                }

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Delete Surface")) delete_surface();
        if (ImGui::Button("Clear Surface"))  clear_surface();
        if (ImGui::Button("Enable Brush"))   switch_mode_brush();
    }

    switch (settings.interact_mode) {
        case InteractMode::Idle: {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + gui_width + 5, main_viewport->WorkPos.y + 5));
            ImGui::Begin("Mode: Idle", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Text("These controls also work in other modes.");
            ImGui::Bullet(); ImGui::Text("<RMB> and drag to rotate the camera.");
            ImGui::Bullet(); ImGui::Text("<Shift-LMB> and drag to pan.");
            ImGui::Bullet(); ImGui::Text("<Scroll> to zoom in and out.");
            ImGui::Bullet(); ImGui::Text("<E> to toggle the GUI.");
            ImGui::End();
        } break;
        case InteractMode::DrawPSLG: {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + gui_width + 5, main_viewport->WorkPos.y + 5));
            ImGui::Begin("Mode: PSLG Drawing", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Bullet(); ImGui::Text("<LMB> points in sequence to draw connected line segments.");
            ImGui::Bullet(); ImGui::Text("<Ctrl-Enter> to finalize a contiguous section of the drawing.");
            ImGui::Bullet(); ImGui::Text("<Enter> to finalize the entire drawing.");
            ImGui::End();
        } break;
        case InteractMode::AddHole: {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + gui_width + 5, main_viewport->WorkPos.y + 5));
            ImGui::Begin("Mode: Add Hole", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Text("<LMB> in a closed region to designate it as a hole.");
            ImGui::End();
        } break;
        case InteractMode::Brush: {
            ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + gui_width + 5, main_viewport->WorkPos.y + 5));
            ImGui::Begin("Mode: Brush", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
            ImGui::Text("<LMB> on elements to set the values of their nodes.");
            ImGui::End();
        } break;
    }

    ImGui::End();
}

/**
 * The program's main loop.
 */
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
        if (solver->surface)
            solver->advance_time();

        render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
    }
}

void Application::reset_orbit_position() {
    camera->set_orbit_position(glm::vec3(0.0f));
}
void Application::align_top_down() {
    camera->align_to_plane();
}
void Application::clear_pslg() {
    pslg->clear();
}
void Application::clear_holes() {
    pslg->clear_holes();
}
void Application::clear_surface() {
    surface->clear_values();
}
void Application::delete_surface() {
    surface->clear();
    solver->surface = nullptr;
    bvh = nullptr;
}
void Application::init_surface_from_pslg() {
    clear_surface(); 

    surface->init_from_PSLG(*pslg);
    solver->surface = surface;
    solver->init();
    bvh = std::make_unique<BVH>(surface, settings.bvh_depth);

    clear_pslg();
}
void Application::init_surface_from_obj() {
    clear_pslg();
    clear_surface(); 

    nfdchar_t *out_path = NULL;		
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &out_path);
    surface->init_from_obj(out_path);
    solver->surface = surface;
    solver->init();
    bvh = std::make_unique<BVH>(surface, settings.bvh_depth);
}
void Application::switch_solver(SolverType new_solver) {
    bool valid_new_solver = false;

    switch (new_solver) {
        case SolverType::Heat: 
            solver = std::make_shared<HeatSolver>();
            valid_new_solver = true;
            break;
        case SolverType::Wave: 
            solver = std::make_shared<WaveSolver>();
            valid_new_solver = true;
            break;
        case SolverType::Advection_Diffusion: 
            solver = std::make_shared<AdvectionDiffusionSolver>();
            valid_new_solver = true;
            break;
    }

    if (valid_new_solver && surface->initialized) {
        solver->surface = surface;
        solver->init();
    }
}
void Application::switch_color_map(const char* new_color_map) {
    surface->color_map = color_maps.get(new_color_map);
}

void Application::switch_mode_draw_pslg() {
    camera->align_to_plane();
    settings.interact_mode = InteractMode::DrawPSLG;
}
void Application::switch_mode_add_hole() {
    settings.interact_mode = InteractMode::AddHole;
}
void Application::switch_mode_brush() {
    settings.interact_mode = InteractMode::Brush;
}

/**
 * Set the value of some region on the surface given a world ray and origin.
 * Right now, this sets the value of the closest vertex to the intersection point. 
 * Note that the ray must intersect the surface in order for a value to be set. 
 * 
 * @param world_ray The normalized direction vector of the ray derived from mouse picking.
 * @param origin The origin of the world ray.
 * @param value The value to set each of the nodal values to. 
 */
void Application::brush(glm::vec3 world_ray, glm::vec3 origin, float value) {
    RayTriangleIntersection intersection = bvh->ray_triangle_intersection(origin, world_ray);

    if (intersection.tri_idx != -1) {
        int point_idx = -1;
        float min_point_dist = 0.0f;
        for (int i = 0; i < 3; i++) {
            float dist = glm::distance(surface->vertices[surface->triangles[intersection.tri_idx][i]], intersection.point);
            if ((point_idx == -1 || dist < min_point_dist) && dist > 0.0f) {
                min_point_dist = dist;
                point_idx = i;
            }
        }
        
        if (point_idx != -1) {
            surface->values[surface->triangles[intersection.tri_idx][point_idx]] = value;
        }
    }
}

/**
 * Returns the direction of the ray in 3D space that is created by the mouse.
 */
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

/**
 * Returns the intersection of the ray created by the mouse and the XZ plane.
 */
glm::vec3 Application::get_mouse_to_grid_plane_point() {
    glm::vec3 world_ray = get_world_ray_from_mouse();
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 origin = camera->get_camera_position();

    float parameter = -glm::dot(normal, origin) / glm::dot(normal, world_ray);
    glm::vec3 intersection = origin + world_ray * parameter;
    return intersection;
}

void Application::init_opengl_window(unsigned int window_width, unsigned int window_height) {
    this->window_width = window_width;
    this->window_height = window_height;

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

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
        app->camera->pan(-dx, -dy);
    }
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        app->camera->rotate(dx, dy);
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }


    switch (app->settings.interact_mode) {
        case InteractMode::Brush: {
            if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse && !(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)) {
                app->brush(app->get_world_ray_from_mouse(), app->camera->get_camera_position(), 1.0f);
            }
        } break;

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
        case InteractMode::Brush: {
            if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
                app->settings.interact_mode = InteractMode::Idle;
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
                app->brush(app->get_world_ray_from_mouse(), app->camera->get_camera_position(), 1.0f);
            }
        } break;
    }
}