#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>

#include "Shader.hpp"
#include "Camera.hpp"
#include "GridVisual.hpp"

#include <iostream>
#include <memory>

unsigned int WINDOW_WIDTH = 1000;
unsigned int WINDOW_HEIGHT = 800;
float r = (float)WINDOW_WIDTH / WINDOW_HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_pos_callback(GLFWwindow* window, double x, double y);
void scroll_callback(GLFWwindow* window, double dx, double dy);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void process_input(GLFWwindow* window);

Camera camera;
bool mouse = true;

std::unique_ptr<Shader> default_shader;
std::unique_ptr<Shader> solid_color_shader;
std::unique_ptr<Shader> vertex_color_shader;

int main() {
	float vertices[] = {
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,   // top right
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom right
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom left
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f    // top left 
	};

	unsigned int indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "FEA Visualizer", NULL, NULL);
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glEnable(GL_DEPTH_TEST);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = NULL;
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	unsigned int texture;
	int width, height, channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load("assets/aurora.png", &width, &height, &channels, 0);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	float brightness = 1.0f;

    default_shader = std::make_unique<Shader>("shaders/default_vert.glsl", "shaders/default_frag.glsl"); 
    solid_color_shader = std::make_unique<Shader>("shaders/solid_color_vert.glsl", "shaders/solid_color_frag.glsl"); 
    vertex_color_shader = std::make_unique<Shader>("shaders/vertex_color_vert.glsl", "shaders/vertex_color_frag.glsl"); 

    GridVisual grid_visual;

    camera.set_aspect_ratio((float)WINDOW_WIDTH / WINDOW_HEIGHT);
    default_shader->bind();
	default_shader->set_int("tex", 0);

	while (!glfwWindowShouldClose(window)) {
		process_input(window);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui Stuff Goes Here
		// ImGui::Begin("Menu");
		// ImGui::SliderFloat("Brightness", &brightness, 0.0f, 1.0f);
		// ImGui::End();

        vertex_color_shader->bind();
        vertex_color_shader->set_mat4x4("model", glm::mat4(1.0f));
        vertex_color_shader->set_mat4x4("view_proj", camera.get_view_projection_matrix());
        grid_visual.draw_grid(*vertex_color_shader);

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            solid_color_shader->bind();
            solid_color_shader->set_mat4x4("view_proj", camera.get_view_projection_matrix());
            grid_visual.draw_panning_locator(*solid_color_shader, camera.get_orbit_position());
        }

		default_shader->bind();
		default_shader->set_float("brightness", brightness);
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        default_shader->set_mat4x4("model", model);
        default_shader->set_mat4x4("view_proj", camera.get_view_projection_matrix());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}

void process_input(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);	
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;
}

void cursor_pos_callback(GLFWwindow* window, double x, double y) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window); 

	static float last_x = (float)WINDOW_WIDTH / 2.0;
	static float last_y = (float)WINDOW_HEIGHT / 2.0;
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

	if (!mouse) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            camera->pan(dx, dy);
        } else {
            camera->rotate(dx, dy);
        }
	}
}

void scroll_callback(GLFWwindow* window, double dx, double dy) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window); 

    if (!mouse) {
        camera->zoom(dy);
		camera->rotate(dx, 0);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window); 

	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		mouse = !mouse;
		glfwSetInputMode(window, GLFW_CURSOR, mouse ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        if (!mouse)
            camera->set_perspective();            
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        if (!mouse)
            camera->set_orthographic();            
	}
	if (key == GLFW_KEY_0 && action == GLFW_PRESS) {
		if (!mouse)
			camera->set_orbit_position(glm::vec3(0.0f));
	}
}