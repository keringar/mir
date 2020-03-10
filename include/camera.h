#ifndef CAMERA_H
#define CAMERA_H

#include "math.hpp"
#include "rng.h"
#include "ray.h"

class Camera {

private:
    int m_width;
    double m_width_recp;
    int m_height;
    double m_height_recp;
    double m_ratio;
    double m_x_spacing;
    double m_x_spacing_half;
    double m_y_spacing;
    double m_y_spacing_half;
    float3 m_position;
    float3 m_direction;
    float3 m_x_direction;
    float3 m_y_direction;

public:
    Camera(float3 position, float3 target, int width, int height);
    int get_width();
    int get_height();
    Ray get_ray(int x, int y, bool jitter, Rng& rng);
};

#endif //CAMERA_H