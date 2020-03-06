#include <iostream>

#include "Volume.hpp"
#include "filesystem.hpp"

Volume::Volume() {
    this->m_width = 0;
    this->m_height = 0;
    this->m_depth = 0;
    this->m_size = float3(1.f, 1.f, 1.f);
}

Volume::Volume(uint32_t width, uint32_t height, uint32_t depth) {
    this->m_width = width;
    this->m_height = height;
    this->m_depth = depth;
    this->m_size = float3(1.f, 1.f, 1.f);

    data = std::vector<uint16_t>(width * height * depth, 0);
}

void Volume::set_size(float3 size) {
    this->m_size = size;
}

uint16_t* Volume::get_raw_data() {
    return data.data();
}

uint16_t Volume::sample_at(float3 world_pos) {
    if (world_pos.x < m_size.x / 2 && world_pos.x > -m_size.x / 2) {
        if (world_pos.y < m_size.y / 2 && world_pos.y > -m_size.y / 2) {
            if (world_pos.z < m_size.z / 2 && world_pos.z > -m_size.z / 2) {
                // Resize volume to between 0 and size
                world_pos += m_size / 2.f;

                // Calculate sample point from world_pos
                uint32_t index_x = min((uint32_t)(m_width * (world_pos.x / m_size.x)), m_width);
                uint32_t index_y = min((uint32_t)(m_height * (world_pos.y / m_size.y)), m_height);
                uint32_t index_z = min((uint32_t)(m_depth * (world_pos.z / m_size.z)), m_depth);
                return data[index_x + m_width * (index_y + m_depth * index_z)];
            }
        }
    }

    return 0;
}

float3 Volume::gradient_at(float3 world_pos) {
    float3 voxel_size = float3(m_size.x / m_width, m_size.y / m_height, m_size.z / m_depth);

    uint16_t sample = sample_at(world_pos);
    uint16_t gradient_x = sample_at(float3(world_pos.x + voxel_size.x, world_pos.y, world_pos.z)) - sample;
    uint16_t gradient_y = sample_at(float3(world_pos.x, world_pos.y + voxel_size.y, world_pos.z)) - sample;
    uint16_t gradient_z = sample_at(float3(world_pos.x, world_pos.y, world_pos.z + voxel_size.z)) - sample;

    return normalize(float3(gradient_x, gradient_y, gradient_z));
}

float3 Volume::tangent_at(float3 world_pos) {
    float3 normal = gradient_at(world_pos);

    // I think we can pick any perpendicular angle to the normal?
    // There's no textures so we don't need to consider that?
    // TODO: handle case when -normal.x = normal.y
    return normalize(float3(normal.z, normal.z, -normal.x - normal.y));
}
