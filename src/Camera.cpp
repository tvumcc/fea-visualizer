#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"

#include <iostream>
#include <format>

Camera::Camera() {
    update_camera_position();
}

Camera::Camera(glm::vec3 orbit_position, float aspect_ratio, float radius, float yaw, float pitch) :
    orbit_position(orbit_position),
    aspect_ratio(aspect_ratio),
    radius(radius),
    yaw(yaw),
    pitch(pitch)
{
    update_camera_position();
}

/**
 * Rotate the camera about its orbit point.
 * 
 * @param dx The change in the x position, presumably from the mouse
 * @param dy The change in the y position, presumable from the mouse
 */
void Camera::rotate(float dx, float dy) {
    yaw += rotation_sensitivity * dx;

    // Limit vertical orbiting
    float new_pitch = pitch + rotation_sensitivity * dy;
    if (new_pitch + dy <= 90.0f && new_pitch + dy >= -90.0f) {
        pitch = new_pitch;
    } 

    update_camera_position();
}

/**
 * Change the distance between the camera's position and the orbit point.
 * 
 * @param dr The change in radius
 */
void Camera::zoom(float dr) {
    // Limit zoom from passing through the center of orbit
    float new_radius = radius + zoom_sensitivity * dr;
    if (new_radius > 0.0f) {
        radius = new_radius;
    }

    update_camera_position();
}

/**
 * Move the camera's orbit point depending on what direction the camera is facing.
 * 
 * @param dx The change in the x position, presumably from the mouse
 * @param dy The change in the y position, presumably from the mouse
 */
void Camera::pan(float dx, float dy) {
    glm::vec3 camera_direction = get_facing_direction();

    glm::vec3 camera_right = glm::normalize(glm::cross(up, camera_direction));
    glm::vec3 camera_up = glm::normalize(glm::cross(camera_right, camera_direction));

    orbit_position += camera_right * dx * pan_sensitivity;
    orbit_position += camera_up * dy * pan_sensitivity;

    update_camera_position();
}

/**
 * Return the product of the camera's projection and view matrices.
 */
glm::mat4 Camera::get_view_projection_matrix() {
    return get_projection_matrix() * get_view_matrix();
}
/**
 * Return the camera's 4x4 view matrix
 */
glm::mat4 Camera::get_view_matrix() {
    return glm::lookAt(camera_position, orbit_position, up);
}
/**
 * Return the camera's 4x4 perspective projection matrix.
 */
glm::mat4 Camera::get_projection_matrix() {
    return glm::perspective(fov, aspect_ratio, z_near, z_far);
}

/**
 * Return the normalized vector representing the direction that the camera is currently facing.
 */
glm::vec3 Camera::get_facing_direction() {
    return glm::normalize(orbit_position - camera_position);
}

/**
 * Return the current position around which the camera is orbiting.
 */
glm::vec3 Camera::get_orbit_position() {
    return orbit_position;
}
/**
 * Return the camera's position in 3D space.
 */
glm::vec3 Camera::get_camera_position() {
    return camera_position;
}

/**
 * Set the aspect ratio of the camera for use in projection matrix calculations.
 * 
 * @param new_aspect_ratio The new aspect ratio of the camera
 */
void Camera::set_aspect_ratio(float new_aspect_ratio) {
    this->aspect_ratio = new_aspect_ratio;
}

/**
 * Set the orbit position of the camera. This is the point around which the camera is free to rotate around. 
 * 
 * @param orbit_position The new orbital center of the camera
 */
void Camera::set_orbit_position(glm::vec3 new_orbit_position) {
    this->orbit_position = new_orbit_position;
    update_camera_position();
}

/**
 * Set the camera's position and direction so that it is facing the XZ plane on the +Y side.
 */
void Camera::align_to_plane() {
    yaw = 90.0f;
    pitch = 89.999f;
    update_camera_position();
}

/**
 * Update the camera's position in 3D world space based off of its distance and relative orientation
 * to the orbit point.
 */
void Camera::update_camera_position() {
	camera_position = glm::vec3(
		orbit_position.x + radius * glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch)),
		orbit_position.y + radius * glm::sin(glm::radians(pitch)),
		orbit_position.z + radius * glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch))
	);
}