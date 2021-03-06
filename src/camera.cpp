#include "ray.h"
#include "camera.h"

Camera::Camera(float3 position, float3 target, float3 up, int width, int height) {
    m_width = width;
    m_width_recp = 1.f / m_width;
    m_height = height;
    m_height_recp = 1.f / m_height;
    m_ratio = (float)m_width / m_height;

    m_position = position;
    m_direction = normalize(target - m_position);
    m_x_direction = normalize(cross(up, m_direction * -1));
    m_y_direction = normalize(cross(m_x_direction, m_direction));

    m_x_spacing = (2.0f * m_ratio) / (float)m_width;
    m_y_spacing = 2.0f / (float)m_height;
    m_x_spacing_half = m_x_spacing * 0.5f;
    m_y_spacing_half = m_y_spacing * 0.5f;
}

int Camera::get_width() { return m_width; }
int Camera::get_height() { return m_height; }

// Returns ray from camera origin through pixel at x,y
Ray Camera::get_ray(int x, int y, bool jitter, Rng &rng) {
    double x_jitter;
    double y_jitter;

    // If jitter == true, jitter point for anti-aliasing
    if (jitter) {
        x_jitter = (rng.generate() * m_x_spacing) - m_x_spacing_half;
        y_jitter = (rng.generate() * m_y_spacing) - m_y_spacing_half;
    }
    else {
        x_jitter = 0.f;
        y_jitter = 0.f;
    }

    float3 pixel = m_position + m_direction * 2.f;
    pixel = pixel - m_x_direction * m_ratio + m_x_direction * ((x * 2.f * m_ratio) * m_width_recp) + x_jitter;
    pixel = pixel + m_y_direction - m_y_direction * ((y * 2.0f) * m_height_recp + y_jitter);

    Ray ray;
    ray.origin = m_position;
    ray.direction = normalize(pixel - m_position);

    return ray;
}