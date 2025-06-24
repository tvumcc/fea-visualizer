#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "Application.hpp"

Application::Application() {
    init_opengl_window(1000, 800);
    init_imgui();

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
    camera->set_aspect_ratio((float)window_width / window_height);

    grid_interface = std::make_shared<GridInterface>();
    grid_interface->solid_color_shader = Application::Shaders::solid_color;
    grid_interface->vertex_color_shader = Application::Shaders::vertex_color;
}

void Application::render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    grid_interface->draw(camera, glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
}
void Application::run() {
    while (!glfwWindowShouldClose(window)) {

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

        render();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
    }
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
}
void Application::init_imgui() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    Application* app = (Application*)glfwGetWindowUserPointer(window);

	glViewport(0, 0, width, height);
	app->window_width = width;
	app->window_height = height;
    app->camera->set_aspect_ratio((float)width / height);
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

	if (!app->mouse_activated) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            app->camera->pan(dx, dy);
        } else {
            app->camera->rotate(dx, dy);
        }
	}
}
void scroll_callback(GLFWwindow* window, double dx, double dy) {
    Application* app = (Application*)glfwGetWindowUserPointer(window); 

    if (!app->mouse_activated) {
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
		app->mouse_activated = !app->mouse_activated;
		glfwSetInputMode(window, GLFW_CURSOR, app->mouse_activated ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (!app->mouse_activated)
            app->camera->set_perspective();            
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        if (!app->mouse_activated)
            app->camera->set_orthographic();            
	}
	if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
		if (!app->mouse_activated)
			app->camera->set_orbit_position(glm::vec3(0.0f));
	}
}