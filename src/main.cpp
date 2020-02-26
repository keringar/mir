#include "Dicom.hpp"
#include "math.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <random>
#include <cmath>

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
    float3 origin;
};

struct ScatterEvent {
    bool valid;
    uint16_t sample;
    float3 position;
    DisneyMaterial mat;
};

DisneyMaterial sample_to_material(uint16_t sample) {
    DisneyMaterial mat = { 0 };
    if (sample > 10000) {
        mat.BaseColor = float3(0.6, 0.1, 0.1);
        mat.Transmission = 0.0f;
    } else {
        // air
        mat.Transmission = 1.0f;
    }

    return mat;
}

ScatterEvent sample_single_ray(const Ray ray, mt19937& rng, std::uniform_real_distribution<>& dis, Volume v) {
    ScatterEvent result = { 0 };

    // Woodcock (Delta-Tracking?) scattering calculation
    // TODO: Play around with step size, the 0.01 is an arbitrary max step size
    float s = 0.f;
    while (true) {
        s += (-log(1.f - dis(rng)) / 1.0f) * 0.01;
        
        float3 current_point = ray.origin + (s * ray.direction);
        if (current_point.z > v.size.z / 2.f) {
            result.valid = false;
            break;
        }

        uint16_t sample = v.sample_at(current_point);
        DisneyMaterial mat = sample_to_material(sample);
        if (dis(rng) < 1.f -  (mat.Transmission / 1.0f)) {
            result.valid = true;
            result.sample = sample;
            result.position = current_point;
            result.mat = mat;
            break;
        }
    }

    return result;

    /*
    while (true) {
        float opaci

        s += -log(1.f - dis(rng)) / MAX_EXTINCTION;

        current_point = ray.origin + (s * ray.direction);
        uint16_t sample = v.sample_at(current_point);
        float opacity = sample_to_material(sample).Opacity;
        if (dis(rng) < (opacity / MAX_EXTINCTION)) {
            break;
        }
    }*/

    /*
    float3 current_point = ray.origin;
    float s = 0.f;
    for (int i = 0; i < 100; i++) {
        s += 0.01f;
        current_point = ray.origin + (ray.direction * s);

        uint16_t sample = v.sample_at(current_point);
        if (sample > 0) {
            break;
        }
    }

    // Sample scatter position
    uint16_t sample = v.sample_at(current_point);

    // Scatter
    ScatterEvent result = { 0 };
    result.valid = sample != 0;
    result.sample = sample;
    result.position = current_point;
    result.mat = sample_to_material(sample);

    return result;
    */
}

float3 sample_light(const ScatterEvent scatter) {
    return scatter.mat.BaseColor;
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

    std::random_device rd;
    mt19937 rng(rd());
    std::uniform_real_distribution<> dis;

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
            ScatterEvent scatter = sample_single_ray(ray, rng, dis, d.volume);
            
            if (scatter.valid) {
                sampled_color = sample_light(scatter);
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
