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
                    uint32_t index_x = (uint32_t)(width * (world_pos.x / size.x));
                    uint32_t index_y = (uint32_t)(height * (world_pos.y / size.y));
                    uint32_t index_z = (uint32_t)(depth * (world_pos.z / size.z));
                    return data[index_x + (index_y * width) + (index_z * width * height)];
                }
            }
        }

        return 0;
    }
};

class Dicom {
    public:
        Volume volume;

        Dicom();
        ~Dicom();

        int LoadDicomStack(const std::string& folder, float3* size);
};
