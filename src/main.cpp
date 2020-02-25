#include "Dicom.hpp"
#include "math.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>

using namespace std;

#define OUTPUT_WIDTH 1024
#define OUTPUT_HEIGHT 1024

struct DisneyMaterial {
    float3 BaseColor;
    float Metallic;
    float3 Emission;
    float Specular;
    float Anisotropy;
    float Roughness;
    float SpecularTint;
    float SheenTint;
    float Sheen;
    float ClearcoatGloss;
    float Clearcoat;
    float Subsurface;
    float Transmission;
    uint32_t pad[3];
};

struct Ray {
    float3 direction;
    float3 position;
};

struct ScatterEvent {
    bool valid;
    bool from_light;
};

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

DisneyMaterial sample_to_material(uint16_t sample) {
    DisneyMaterial mat = { 0 };
    mat.BaseColor = float3(0.6f, 0.f, 0.f);
    mat.Specular = 0.1f;

    return mat;
}

ScatterEvent sample_single_ray(const Ray ray) {

}

float3 sample_light(const ScatterEvent scatter) {

}

float3 tonemap_aces(float3 hdr_color) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((hdr_color * (a * hdr_color + b)) / (hdr_color * (c * hdr_color + d) + e), float3(0.f), float3(1.f));
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

    cout << "Raytracing " << OUTPUT_WIDTH << "x" << OUTPUT_HEIGHT << " image" << endl;
    float3* image = new float3[OUTPUT_WIDTH * OUTPUT_HEIGHT];
    for (uint32_t x = 0; x < OUTPUT_WIDTH; x++) {
        for (uint32_t y = 0; y < OUTPUT_HEIGHT; y++) {
            // Generate ray directions
            const float3 eye = float3(0.f, 0.f, -1.f);
            const float target_x = ((2.f * (float)x / (float)OUTPUT_WIDTH) - 1.f);
            const float target_y = ((2.f * (float)y / (float)OUTPUT_HEIGHT) - 1.f);
            const float3 target = float3(target_x, target_y, eye.z + 4.0f);
            const float3 direction = normalize(target - eye);
            Ray ray = { direction, eye };

            float3 sampled_color = float3(0.f, 0.f, 0.f);
            ScatterEvent scatter = sample_single_ray(ray);
            
            if (scatter.valid) {
                if (scatter.from_light) {
                    sampled_color += float3(1.f, 1.f, 1.f); // add light directly
                } else {
                    sampled_color += sample_light(scatter);
                }
            }

            float3 ldr = tonemap_aces(sampled_color);
            image[x + (y * OUTPUT_HEIGHT)] = pow(ldr, 2.2);
        }
    }

    cout << "Writing output image to HDR file: " << argv[2] << endl;
    stbi_write_hdr(argv[2], OUTPUT_WIDTH, OUTPUT_HEIGHT, 3, (float*)image);
    delete[] image;

    return 0;
}
