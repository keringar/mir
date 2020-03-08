#pragma once

#include "math.hpp"
#include <string>

struct Volume {
    uint16_t* data;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    float3 size;

    uint16_t sample_at(float3 world_pos) {
        if (world_pos.x < size.x / 2 && world_pos.x > -size.x / 2) {
            if (world_pos.y < size.y / 2 && world_pos.y > -size.y / 2) {
                if (world_pos.z < size.z / 2 && world_pos.z > -size.z / 2) {
                    // Resize volume to between 0 and size
                    world_pos += size / 2.f;

                    // Calculate sample point from world_pos
                    uint32_t index_x = min((uint32_t)(width * (world_pos.x / size.x)), width);
                    uint32_t index_y = min((uint32_t)(height * (world_pos.y / size.y)), height);
                    uint32_t index_z = min((uint32_t)(depth * (world_pos.z / size.z)), depth);
                    return data[index_x + width * (index_y + depth * index_z)];
                }
            }
        }

        return 0;
    }

    float3 gradient_at(float3 world_pos) {
        float3 voxel_size = float3(size.x / width, size.y / height, size.z / depth);

        uint16_t sample = sample_at(world_pos);
        uint16_t gradient_x = sample_at(float3(world_pos.x + voxel_size.x, world_pos.y, world_pos.z)) - sample;
        uint16_t gradient_y = sample_at(float3(world_pos.x, world_pos.y + voxel_size.y, world_pos.z)) - sample;
        uint16_t gradient_z = sample_at(float3(world_pos.x, world_pos.y, world_pos.z + voxel_size.z)) - sample;

        return normalize(float3(gradient_x, gradient_y, gradient_z));
    }

    // I think we can pick any perpendicular angle to the normal?
    // There's no textures so we don't need to consider that?
    float3 tangent_at(float3 world_pos) {
        float3 normal = gradient_at(world_pos);

        // TODO: handle case when -normal.x = normal.y
        return normalize(float3(normal.z, normal.z, -normal.x - normal.y));
    }
};

class Dicom {
public:
    Volume volume;

    Dicom();
    ~Dicom();

    int LoadDicomStack(const std::string& folder, float3* size, uint8_t mask_value);
};
