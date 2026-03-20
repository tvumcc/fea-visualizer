#include <glad/glad.h>
#include <stb/stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Utils/Shader.hpp"
#include "Utils/EnvironmentMap.hpp"

#include <iostream>

EnvironmentMap::EnvironmentMap(const char* hdr_image_path) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, num_components;
    float* data = stbi_loadf(hdr_image_path, &width, &height, &num_components, 0);

    int env_map_resolution = 1024;
    int irradiance_map_resolution = 512;
    int prefilter_map_resolution = 1024;
    int brdf_texture_resolution = 512;

    if (data) {
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); // allows filtering across cubemap faces        

        glGenTextures(1, &hdr_texture);
        glBindTexture(GL_TEXTURE_2D, hdr_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);

        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, env_map_resolution, env_map_resolution);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        glGenTextures(1, &env_cubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);
        for (unsigned int i = 0; i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, env_map_resolution, env_map_resolution, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
            {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

        // convert HDR equirectangular environment map to cubemap equivalent
        equirect_to_cube_shader->bind();
        equirect_to_cube_shader->set_int("equirectangular_map", 0);
        equirect_to_cube_shader->set_mat4x4("proj", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdr_texture);

        glViewport(0, 0, env_map_resolution, env_map_resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (int i = 0; i < 6; i++) {
            equirect_to_cube_shader->set_mat4x4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env_cubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cube_mesh->draw(*equirect_to_cube_shader, GL_TRIANGLES);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        glGenTextures(1, &irradiance_map);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map);
        for (int i = 0; i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, irradiance_map_resolution, irradiance_map_resolution, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, irradiance_map_resolution, irradiance_map_resolution);

        irradiance_convolution_shader->bind();
        irradiance_convolution_shader->set_int("environment_map", 0);
        irradiance_convolution_shader->set_mat4x4("proj", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);

        glViewport(0, 0, irradiance_map_resolution, irradiance_map_resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (int i = 0; i < 6; i++) {
            irradiance_convolution_shader->set_mat4x4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance_map, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            cube_mesh->draw(*irradiance_convolution_shader, GL_TRIANGLES);
        }

        // Generating the Pre-filter map
        glGenTextures(1, &prefilter_map);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map);
        for (int i = 0; i < 6; i++)
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefilter_map_resolution, prefilter_map_resolution, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        prefilter_convolution_shader->bind();
        prefilter_convolution_shader->set_int("environment_map", 0);
        prefilter_convolution_shader->set_mat4x4("proj", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        int max_mip_levels = 5;
        for (int mip = 0; mip < max_mip_levels; mip++) {
            int mip_width = prefilter_map_resolution * std::pow(0.5, mip);
            int mip_height = prefilter_map_resolution * std::pow(0.5, mip);
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_width, mip_height);
            glViewport(0, 0, mip_width, mip_height);

            float roughness = (float)mip / (float)(max_mip_levels - 1);
            prefilter_convolution_shader->set_float("roughness", roughness);
            for (int i = 0; i < 6; i++) {
                prefilter_convolution_shader->set_mat4x4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter_map, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                cube_mesh->draw(*prefilter_convolution_shader, GL_TRIANGLES);
            }
        }


        glGenTextures(1, &brdf_texture);
        glBindTexture(GL_TEXTURE_2D, brdf_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdf_texture_resolution, brdf_texture_resolution, 0, GL_RG, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, brdf_texture_resolution, brdf_texture_resolution);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_texture, 0);

        glViewport(0, 0, brdf_texture_resolution, brdf_texture_resolution);
        brdf_convolution_shader->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        quad_mesh->draw(*brdf_convolution_shader, GL_TRIANGLES);


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    } else {
        std::cout << "Failed to load HDR image: " << hdr_image_path << "\n";
    }
}

void EnvironmentMap::draw(std::shared_ptr<Camera> camera) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);

    skybox_shader->bind();
    skybox_shader->set_int("environment_map", 0);
    skybox_shader->set_mat4x4("proj", camera->get_projection_matrix());
    skybox_shader->set_mat4x4("view", camera->get_view_matrix());

    glDepthFunc(GL_LEQUAL);
    cube_mesh->draw(*skybox_shader, GL_TRIANGLES);
    glDepthFunc(GL_LESS);
}

void EnvironmentMap::use(std::shared_ptr<AbstractShader> shader) {
    shader->bind();
    shader->set_int("irradiance_map", 0);
    shader->set_int("prefilter_map", 1);
    shader->set_int("brdf_texture", 2);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brdf_texture);
}