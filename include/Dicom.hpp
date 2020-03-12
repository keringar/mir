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
        if (abs(world_pos.x) < size.x / 2) {
            if (abs(world_pos.y) < size.z / 2) {
                if (abs(world_pos.z) < size.y / 2) {
                    world_pos += float3(size.x, size.z, size.y) / 2;

                    uint32_t slice = min(depth * (world_pos.y / size.z), depth - 1);
                    uint32_t slice_x = width * (world_pos.x / size.x);
                    uint32_t slice_y = height * (world_pos.z / size.y);

                    return data[slice * width * height + slice_y * width + slice_x];
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
    uint16_t max_value;

    Dicom();
    ~Dicom();

    int LoadDicomStack(const std::string& folder, float3* size, bool should_mask, uint8_t mask_value);
};
