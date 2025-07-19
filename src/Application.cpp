#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glm/gtc/matrix_inverse.hpp>
#include <nfd.h>

#include "Application.hpp"
#include "HeatSolver.hpp"
#include "WaveSolver.hpp"
#include "AdvectionDiffusionSolver.hpp"
#include "ReactionDiffusionSolver.hpp"

#include <iostream>
#include <filesystem>

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

    // Color Maps, make sure the name in the ResourceManager and the ColorMap are the same. (as well as the image icon file)
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
    color_maps.add("Inferno", std::make_shared<ColorMap>(
        "Inferno",
        std::array<glm::vec3, 7>({
            glm::vec3(0.000129,0.001094,-0.041044),
            glm::vec3(0.083266,0.574933,4.155398),
            glm::vec3(11.783686,-4.013093,-16.439814),
            glm::vec3(-42.246539,17.689298,45.210269),
            glm::vec3(78.087062,-33.838649,-83.264061),
            glm::vec3(-72.108852,32.950143,74.479447),
            glm::vec3(25.378501,-12.368929,-23.407604),
        })
    ));
    color_maps.add("Rainbow", std::make_shared<ColorMap>(
        "Rainbow",
        std::array<glm::vec3, 7>({
            glm::vec3(0.503560,-0.002932,1.000009),
            glm::vec3(-1.294985,3.144463,0.001872),
            glm::vec3(-16.971202,0.031355,-1.232219),
            glm::vec3(97.134102,-5.180126,-0.029721),
            glm::vec3(-172.585487,-0.338714,0.316782),
            glm::vec3(131.971426,3.514534,-0.061568),
            glm::vec3(-37.784412,-1.171512,0.003376),
        })
    ));
    color_maps.add("Twilight", std::make_shared<ColorMap>(
        "Twilight",
        std::array<glm::vec3, 7>({
            glm::vec3(0.996106,0.851653,0.940566),
            glm::vec3(-6.529620,-0.183448,-3.940750),
            glm::vec3(40.899579,-7.894242,38.569228),
            glm::vec3(-155.212979,4.404793,-167.925730),
            glm::vec3(296.687222,24.084913,315.087856),
            glm::vec3(-261.270519,-29.995422,-266.972991),
            glm::vec3(85.335349,9.602600,85.227117),
        })
    ));
    color_maps.add("Coolwarm", std::make_shared<ColorMap>(
        "Coolwarm",
        std::array<glm::vec3, 7>({
            glm::vec3(0.227376,0.286898,0.752999),
            glm::vec3(1.204846,2.314886,1.563499),
            glm::vec3(0.102341,-7.369214,-1.860252),
            glm::vec3(2.218624,32.578457,-1.643751),
            glm::vec3(-5.076863,-75.374676,-3.704589),
            glm::vec3(1.336276,73.453060,9.595678),
            glm::vec3(0.694723,-25.863102,-4.558659),
        })
    ));
    settings.init_color_maps(color_maps);
    settings.init_equation_textures();
    settings.init_color_map_icon_textures();
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

    switch_solver(SolverType::Reaction_Diffusion);
    switch_color_map("Viridis");

    std::cout << glGetString(GL_VERSION) << "\n";
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
    shaders.get("fem_mesh")->set_float("vertex_extrusion", settings.vertex_extrusion);
    shaders.get("fem_mesh")->set_float("pixel_discard_threshold", settings.pixel_discard_threshold);
    shaders.get("wireframe")->bind();
    shaders.get("wireframe")->set_float("vertex_extrusion", settings.vertex_extrusion);
    surface->draw(settings.draw_surface_wireframe);

    if (settings.draw_grid_interface) grid_interface->draw(camera, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
}

/**
 * Renders the GUI elements with ImGUI.
 */
void Application::render_gui() {
    const ImGuiViewport *main_viewport = ImGui::GetMainViewport();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 6;
    style.WindowRounding = 6;
    style.PopupRounding = 6;
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

    switch (settings.interact_mode) {
        case InteractMode::Idle: {
            if (ImGui::CollapsingHeader("Mode: Idle", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextWrapped("These controls also work in other modes.");
                ImGui::TextWrapped("<RMB> and drag to rotate the camera.");
                ImGui::TextWrapped("<Shift-LMB> and drag to pan.");
                ImGui::TextWrapped("<Scroll> to zoom in and out.");
                ImGui::TextWrapped("<E> to toggle the GUI.");
            }
        } break;
        case InteractMode::DrawPSLG: {
            if (ImGui::CollapsingHeader("Mode: PSLG Drawing", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextWrapped("<LMB> points in sequence to draw connected line segments.");
                ImGui::TextWrapped("<Ctrl-Enter> to finalize a contiguous section of the drawing.");
                ImGui::TextWrapped("<Enter> to finalize the entire drawing.");
            }
        } break;
        case InteractMode::AddHole: {
            if (ImGui::CollapsingHeader("Mode: Add Hole", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextWrapped("<LMB> in a closed region to designate it as a hole.");
            }
        } break;
        case InteractMode::Brush: {
            if (ImGui::CollapsingHeader("Mode: Brush", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::TextWrapped("<LMB> on the mesh to set nodal values.");
            }
        } break;
    }

    ImGui::SeparatorText("Camera");
    if (ImGui::Button("Reset Orbit Position")) reset_orbit_position();
    if (ImGui::Button("Align Top Down"))       align_top_down();
    ImGui::Checkbox("Draw Grid", &settings.draw_grid_interface);

    ImGui::SeparatorText("Surface");
    if (!surface->initialized) {
        if (ImGui::Button("Draw PSLG")) switch_mode(InteractMode::DrawPSLG);
        if (!pslg->empty()) {
            ImGui::Indent();
            if (ImGui::Button("Clear PSLG")) clear_pslg();
            if (pslg->closed()) {
                if (ImGui::Button("Add Hole")) switch_mode(InteractMode::AddHole);
                if (!pslg->holes.empty())
                    if (ImGui::Button("Clear Holes")) clear_holes();
                if (ImGui::Button("Init from PSLG")) init_surface_from_pslg();
            }
            ImGui::Unindent();
        }
        if (ImGui::Button("Init from .obj"))   init_surface_from_obj();
    } else {
        ImGui::Text(std::format("{} nodes", solver->surface->vertices.size()).c_str());
        ImGui::Text(std::format("{} elements", solver->surface->triangles.size()).c_str());
        ImGui::Separator();
        ImGui::Checkbox("Draw Triangle Mesh", &settings.draw_surface_wireframe);
        ImGui::Image(settings.color_map_icon_textures[settings.selected_color_map].first, 
            ImVec2(settings.color_map_icon_textures[settings.selected_color_map].second.x * 1.5, settings.color_map_icon_textures[settings.selected_color_map].second.y * 1.5));
        ImGui::SameLine();
        if (ImGui::BeginCombo("Color Map", settings.color_maps[settings.selected_color_map], ImGuiComboFlags_WidthFitPreview)) {
            for (int i = 0; i < settings.color_maps.size(); i++) {
                ImGui::Image(settings.color_map_icon_textures[i].first, settings.color_map_icon_textures[i].second);
                ImGui::SameLine();

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
        ImGui::SameLine();
        if (settings.interact_mode != InteractMode::Brush) {
            if (ImGui::Button("Enable Brush"))   switch_mode(InteractMode::Brush);
        } else {
            if (ImGui::Button("Disable Brush"))  switch_mode(InteractMode::Idle);
            ImGui::Text("Brush Value");
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##Brush Value", &settings.brush_strength, 0.01f, 1.0f);
        }
        ImGui::Text("Vertex Extrusion");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##Vertex Extrusion", &settings.vertex_extrusion, 0.0f, 1.0f);
        ImGui::Text("Pixel Discard Threshold");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::SliderFloat("##Pixel Discard Threshold", &settings.pixel_discard_threshold, 0.0f, 1.0f);

        ImGui::SeparatorText("Solver");
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

        if (!settings.paused) {
            if (ImGui::Button("Pause")) settings.paused = true;
        } else {
            if (ImGui::Button("Unpause")) settings.paused = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear Solver"))  clear_solver();
        switch ((SolverType)settings.selected_solver) {
            case SolverType::Heat: {
                ImGui::Text("Time Step");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Heat Time Step", &((HeatSolver*)solver.get())->time_step, 0.005f, 0.25f); 
                ImGui::Text("Conductivity");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Heat c", &((HeatSolver*)solver.get())->conductivity, 0.005f, 0.25f);
            } break;
            case SolverType::Wave: {
                ImGui::Text("Time Step");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Wave Time Step", &((WaveSolver*)solver.get())->time_step, 0.005f, 0.25f); 
                ImGui::Text("c");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Wave c", &((WaveSolver*)solver.get())->c, 0.005f, 0.25f);
            } break;
            case SolverType::Advection_Diffusion: {
                AdvectionDiffusionSolver* advection_diffusion_solver = (AdvectionDiffusionSolver*)solver.get();
                ImGui::Text("Time Step");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Advection-Diffusion Time Step", &advection_diffusion_solver->time_step, 0.001f, 0.003f); 
                ImGui::Text("c");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Advection-Diffusion c", &((AdvectionDiffusionSolver*)solver.get())->c, 0.05f, 0.25f);
                ImGui::Text("Velocity");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                if (ImGui::SliderFloat3("##Advection-Diffusion Velocity", advection_diffusion_solver->velocity.data(), -1.0f, 1.0f))
                    advection_diffusion_solver->assemble();
            } break;
            case SolverType::Reaction_Diffusion: {
                ImGui::Text("D_u = 0.08");
                ImGui::Text("D_v = 0.04");
                ImGui::Text("Time Step");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Reaction-Diffusion Time Step", &((ReactionDiffusionSolver*)solver.get())->time_step, 0.001f, 0.010f); 
                ImGui::Text("Feed Rate");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Reaction-Diffusion Feed Rate", &((ReactionDiffusionSolver*)solver.get())->feed_rate, 0.0f, 0.1f); 
                ImGui::Text("Kill Rate");
                ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##Reaction-Diffusion Kill Rate", &((ReactionDiffusionSolver*)solver.get())->kill_rate, 0.0f, 0.1f); 
            } break;
        }
    }

    if (surface->initialized) {
        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowContentSize(ImVec2(0, 0));
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + gui_width + 5, main_viewport->WorkPos.y + 5));
        ImGui::Begin(std::format("{} Equation:", settings.solvers[settings.selected_solver]).c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize |ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));
        ImGui::Image(settings.solver_equation_textures[settings.selected_solver].first, settings.solver_equation_textures[settings.selected_solver].second);
        ImGui::End();
    }
}

/**
 * The program's main loop.
 */
void Application::run() {
    while (!glfwWindowShouldClose(window)) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

        const ImGuiViewport *main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x, main_viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(gui_width, main_viewport->WorkSize.y));
        ImGui::Begin("Finite Element Visualizer", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | (!gui_visible ? ImGuiWindowFlags_NoScrollWithMouse : 0));

        if (settings.interact_mode == InteractMode::DrawPSLG)
            pslg->set_pending_point(get_mouse_to_grid_plane_point());
        if (ImGui::GetIO().WantCaptureMouse)
            pslg->pending_point.reset();
        if (settings.interact_mode == InteractMode::Brush && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !ImGui::GetIO().WantCaptureMouse && !(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS))
            brush(get_world_ray_from_mouse(), camera->get_camera_position(), settings.brush_strength);
        if (solver->surface && !settings.paused) {
            solver->advance_time();
            if (solver->has_numerical_instability()) {
                solver->clear_values();
                settings.error_message = "Numerical instability detected! Try changing the solver's parameters.\n Clearing solver values...";
                ImGui::OpenPopup("Error");
            }
        }

        if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Text(settings.error_message.c_str());
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

        if (gui_visible)
            render_gui();
        render();
        ImGui::End();

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
void Application::clear_solver() {
    solver->clear_values();
    surface->clear_values();
}
void Application::delete_surface() {
    surface->clear();
    solver->surface = nullptr;
    bvh = nullptr;
}
void Application::init_surface_from_pslg() {
    delete_surface(); 

    surface->init_from_PSLG(*pslg);
    solver->surface = surface;
    solver->init();
    bvh = std::make_unique<BVH>(surface, settings.bvh_depth);
    switch_mode(InteractMode::Brush);

    clear_pslg();
}
void Application::init_surface_from_obj() {
    clear_pslg();
    delete_surface(); 

    nfdchar_t *out_path = NULL;		
    nfdresult_t result = NFD_OpenDialog(NULL, NULL, &out_path);
    if (!std::filesystem::exists(out_path)) return;
    try {
        surface->init_from_obj(out_path);
        solver->surface = surface;
        solver->init();
        bvh = std::make_unique<BVH>(surface, settings.bvh_depth);
        switch_mode(InteractMode::Brush);
    } catch (std::runtime_error& e) {
        settings.error_message = e.what();
        ImGui::OpenPopup("Error");
    }
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
        case SolverType::Reaction_Diffusion:
            solver = std::make_shared<ReactionDiffusionSolver>();
            valid_new_solver = true;
            break;
    }

    if (valid_new_solver) {
        if (surface->initialized) {
            solver->surface = surface;
            solver->init();
        }
        settings.selected_solver = (int)new_solver;
    }
}
void Application::switch_color_map(const char* new_color_map) {
    for (int i = 0; i < settings.color_maps.size(); i++) {
        if (std::string(settings.color_maps[i]) == std::string(new_color_map)) {
            settings.selected_color_map = i;
            surface->color_map = color_maps.get(new_color_map);
        }
    }
}
void Application::switch_mode(InteractMode mode) {
    switch (mode) {
        case InteractMode::Idle:
            settings.interact_mode = InteractMode::Idle;
            break;
        case InteractMode::DrawPSLG:
            camera->align_to_plane();
            settings.interact_mode = InteractMode::DrawPSLG;
            break;
        case InteractMode::AddHole:
            settings.interact_mode = InteractMode::AddHole;
            break;
        case InteractMode::Brush:
            settings.interact_mode = InteractMode::Brush;
            break;
    }
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(window_width, window_height, "Finite Element Visualizer", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetWindowUserPointer(window, this);
    set_glfw_callbacks();

    // Set the icon if it exists, otherwise, no sweat :)
    if (std::filesystem::exists("assets/icon.png")) {
        GLFWimage images[1];
        int width, height, channels;
        images[0].pixels = stbi_load("assets/icon.png", &width, &height, &channels, 0);
        images[0].width = width;
        images[0].height = height;
        glfwSetWindowIcon(window, 1, images);
    }
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

    if (!ImGui::GetIO().WantCaptureMouse) {
        app->camera->zoom(dy);
        app->camera->rotate(dx, 0);
    }
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
    }
}