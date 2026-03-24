#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <stb/stb_image.h>

#include "Utils/AssetStorage.hpp"

#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <unordered_map>
#include <string>

std::shared_ptr<Mesh> AssetStorage::get_mesh(std::string name) {
    return meshes.get(name);
}

std::shared_ptr<Shader> AssetStorage::get_shader(std::string name) {
    return std::static_pointer_cast<Shader>(shaders.get(name));
}

std::shared_ptr<ComputeShader> AssetStorage::get_compute_shader(std::string name) {
    return std::static_pointer_cast<ComputeShader>(shaders.get(name));
}

std::shared_ptr<ColorMap> AssetStorage::get_color_map(std::string name) {
    return color_maps.get(name);
}

std::shared_ptr<EnvironmentMap> AssetStorage::get_environment_map(std::string name) {
    return environment_maps.get(name);
}

void AssetStorage::load_all_assets() {
    init_background_texture("assets/loading_bg.png");

    load_meshes();
    for (float i = 0.0; i < 0.2; i += 0.01) {
        render_loading_screen(i, "Loading Meshes");
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
    }

    load_shaders();
    for (float i = 0.2; i < 0.4; i += 0.01) {
        render_loading_screen(i, "Loading Shaders");
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
    }

    load_color_maps();
    load_environment_maps();
}

void AssetStorage::render_loading_screen(float progress, const char* message) {
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport *mv = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(mv->WorkPos.x, mv->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(mv->WorkSize.x, mv->WorkSize.y));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Image", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    float viewport_w = mv->WorkSize.x, viewport_h = mv->WorkSize.y;
    float scale = std::max(viewport_w / bg_width, viewport_h / bg_height);
    float scaled_w = bg_width * scale, scaled_h = bg_height * scale;

    float uv_w = viewport_w / scaled_w, uv_h = viewport_h / scaled_h;
    float uv_x = (1.0f - uv_w) / 2.0f, uv_y = (1.0f - uv_h) / 2.0f;

    ImVec2 uv0(uv_x, uv_y);
    ImVec2 uv1(uv_x + uv_w, uv_y + uv_h);

    ImGui::Image((ImTextureID)(intptr_t)bg_texture, ImVec2(viewport_w, viewport_h), uv0, uv1);

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::SetNextWindowPos(ImVec2(mv->WorkPos.x + mv->WorkSize.x / 2 - 250, mv->WorkPos.y + mv->WorkSize.y / 2 - 40));
    ImGui::SetNextWindowSize(ImVec2(500, 80));
    ImGui::Begin("Loading Bar", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    ImGui::ProgressBar(progress);
    ImGui::Text(message);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void AssetStorage::init_background_texture(const char* path) {
    int width, height, channels;
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &bg_texture);
    glBindTexture(GL_TEXTURE_2D, bg_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);

    bg_width = (float)width;
    bg_height = (float)height;
}

void AssetStorage::load_meshes() {
    meshes.add("sphere", std::make_shared<Mesh>("assets/meshes/sphere.obj"));
    meshes.add("quad", std::make_shared<Mesh>("assets/meshes/quad.obj"));
    meshes.add("cube", std::make_shared<Mesh>("assets/meshes/cube.obj"));
}

void AssetStorage::load_shaders() {
    shaders.add("solid_color", std::make_shared<Shader>("shaders/solid_color.vert", "shaders/solid_color.frag"));
    shaders.add("textured", std::make_shared<Shader>("shaders/solid_color.vert", "shaders/textured.frag"));
    shaders.add("vertex_color", std::make_shared<Shader>("shaders/vertex_color.vert", "shaders/vertex_color.frag"));
    shaders.add("wireframe", std::make_shared<Shader>("shaders/PBR/fem_mesh.vert", "shaders/solid_color.frag"));

    shaders.add("fem_mesh", std::make_shared<Shader>("shaders/PBR/fem_mesh.vert", "shaders/PBR/fem_mesh.frag"));
    shaders.add("equirect_to_cube", std::make_shared<Shader>("shaders/PBR/equirect_to_cube.vert", "shaders/PBR/equirect_to_cube.frag"));
    shaders.add("skybox", std::make_shared<Shader>("shaders/PBR/skybox.vert", "shaders/PBR/skybox.frag"));
    shaders.add("irradiance_convolution", std::make_shared<Shader>("shaders/PBR/skybox.vert", "shaders/PBR/irradiance_convolution.frag"));
    shaders.add("prefilter_convolution", std::make_shared<Shader>("shaders/PBR/skybox.vert", "shaders/PBR/prefilter_convolution.frag"));
    shaders.add("brdf_convolution", std::make_shared<Shader>("shaders/PBR/ndc.vert", "shaders/PBR/brdf_convolution.frag"));

    shaders.add("cgm", std::make_shared<ComputeShader>("shaders/FEM/cgm.glsl"));
    shaders.add("cgm_helper", std::make_shared<ComputeShader>("shaders/FEM/cgm_helper.glsl"));
    shaders.add("smooth_normals", std::make_shared<ComputeShader>("shaders/FEM/smooth_normals.glsl"));
}

void AssetStorage::load_color_maps() {
    // Make sure the name in the ResourceManager and the ColorMap are the same. (as well as the image icon file)
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
}

void AssetStorage::load_environment_maps() {
    EnvironmentMap::equirect_to_cube_shader = get_shader("equirect_to_cube");
    EnvironmentMap::skybox_shader = get_shader("skybox");
    EnvironmentMap::irradiance_convolution_shader = get_shader("irradiance_convolution");
    EnvironmentMap::prefilter_convolution_shader = get_shader("prefilter_convolution");
    EnvironmentMap::brdf_convolution_shader = get_shader("brdf_convolution");
    EnvironmentMap::cube_mesh = get_mesh("cube");
    EnvironmentMap::quad_mesh = get_mesh("quad");

    std::unordered_map<std::string, std::string> map = {
        {"lakeside", "assets/hdr_images/lakeside_night_4k.hdr"},
    };

    int idx = 0;
    int sz = map.size();
    float portion = 0.6f / sz;
    for (auto kp : map) {
        float min_progress = 0.4f + portion * idx;
        float max_progress = 0.4f + portion * (idx + 1);

        render_loading_screen(min_progress, "Loading Environment Maps");
        load_environment_map(kp.first.c_str(), kp.second.c_str(), min_progress, max_progress);
        render_loading_screen(max_progress, "Loading Environment Maps");

        idx++;
    }
}

void AssetStorage::load_environment_map(const char* name, const char* hdr_image_path, float min_progress, float max_progress) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, num_components;
    float* data = stbi_loadf(hdr_image_path, &width, &height, &num_components, 0);
    render_loading_screen(0.5f * (min_progress + max_progress), "Loading Environment Maps");

    if (data)
        environment_maps.add("lakeside", std::make_shared<EnvironmentMap>(data, width, height));
}