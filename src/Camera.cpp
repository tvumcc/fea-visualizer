#include <glm/gtc/matrix_transform.hpp>

#include "Camera.hpp"

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
    if (perspective) {
        yaw += rotation_sensitivity * dx;

        // Limit vertical orbiting
        float new_pitch = pitch + rotation_sensitivity * dy;
        if (new_pitch + dy < 89.0f && new_pitch + dy > -89.0f) {
            pitch = new_pitch;
        } 
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
 * @param dy The change in the y position, presumable from the mouse
 */
void Camera::pan(float dx, float dy) {
    glm::vec3 camera_direction = get_facing_direction();
    glm::vec3 right = glm::cross(camera_direction, up);
    glm::vec3 above = glm::cross(right, camera_direction);

    orbit_position += right * dx * pan_sensitivity;
    orbit_position += above * dy * pan_sensitivity;

    update_camera_position();
}

/**
 * Return the product of the projection and view matrices represented by this camera object.
 */
glm::mat4 Camera::get_view_projection_matrix() {
    glm::mat4 view = glm::lookAt(camera_position, orbit_position, up);
    glm::mat4 proj;
    if (perspective) {
        proj = glm::perspective(fov, aspect_ratio, z_near, z_far);
    } else {
        proj = glm::ortho(-aspect_ratio * radius, aspect_ratio * radius, -1.0f * radius, 1.0f * radius, z_near, z_far);
    }
    return proj * view;
}

/**
 * Return the normalized direction that the camera is currently facing
 */
glm::vec3 Camera::get_facing_direction() {
    return glm::normalize(orbit_position - camera_position);
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
 * Changes the camera to use a perspective projection matrix that takes into account distance.
 */
void Camera::set_perspective() {
    perspective = true;
    update_camera_position();
}

/**
 * Changes the camera to use an orthographic projection matrix that does not take into account distance.
 * Also aligns the camera direction to face in the -z direction and prevents rotating of the camera (but allows panning and zooming in and out).
 */
void Camera::set_orthographic() {
    perspective = false;
    yaw = 90.0f;
    pitch = 0.0f;
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