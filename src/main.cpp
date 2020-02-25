#include "Dicom.hpp"
#include "math.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>

using namespace std;

#define OUTPUT_WIDTH 512
#define OUTPUT_HEIGHT 512

uint16_t sample_at(uint16_t* data, uint32_t width, uint32_t height, uint32_t depth, float3 size, float3 world_pos) {
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

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Usage: mir <data folder> <output filename>" << endl;
        return 0;
    }

    float3 size(1.f, 1.f, 1.f);
    Dicom d;
    if (d.LoadDicomStack(argv[1], &size)) {
        cerr << "FATAL: Error loading Dicom stack" << endl;
        return -1;
    } else {
        cout << "Successfully loaded DCM stack" << endl;
    }

    const float3 eye = float3(0.f, 0.f, -1.f);
    const float fov = 90.f;
    const float width = 

    cout << "Raytracing " << OUTPUT_WIDTH << "x" << OUTPUT_HEIGHT << " image" << endl;
    float3* image = new float3[OUTPUT_WIDTH * OUTPUT_HEIGHT];
    for (uint32_t x = 0; x < OUTPUT_WIDTH; x++) {
        for (uint32_t y = 0; y < OUTPUT_HEIGHT; y++) {
            const float3 eye = float3(0.f, 0.f, -1.f);
            const float fov = 90.f;

            const float3 target = float3(x - (float)(d.width / 2), y - (float)(d.height / 2), 0.0);
            const float3 direction = normalize(target - eye);

            // Ray trace
            float distance = 0.f;
            float3 sampled_color = 0.f;
            for (int i = 0; i < 1000; i++) {
                distance += 0.01f;
                float3 point = eye + (direction * distance);

                uint16_t sample = sample_at(d.data, d.width, d.height, d.depth, size, point);
                if (sample > 0) {
                    sampled_color = float3(1.f, 1.f, 1.f);
                    break;
                }
            }

            image[x + (y * OUTPUT_HEIGHT)] = sampled_color;
        }
    }

    cout << "Writing output image to HDR file: " << argv[2] << endl;
    stbi_write_hdr(argv[2], OUTPUT_WIDTH, OUTPUT_HEIGHT, 3, (float*)image);
    delete[] image;

    return 0;
}
