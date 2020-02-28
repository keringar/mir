#include "Dicom.hpp"
#include "math.hpp"
#include "disney.h"
#include "rng.h"
#include "plf.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <cmath>

using namespace std;

#define OUTPUT_WIDTH 1024
#define OUTPUT_HEIGHT 1024

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

ScatterEvent sample_single_ray(const Ray ray, Rng& rng, Volume v, PLF& plf) {
    ScatterEvent result = { 0 };

    // Woodcock (Delta-Tracking?) scattering calculation
    // TODO: Play around with step size, the 0.01 is an arbitrary max step size
    float s = 0.f;
    while (true) {
        s += (-log(1.f - rng.generate()) / 1.0f) * 0.01;
        
        float3 current_point = ray.origin + (s * ray.direction);
        if (current_point.z > v.size.z / 2.f) {
            result.valid = false;
            break;
        }

        uint16_t sample = v.sample_at(current_point);
        DisneyMaterial mat = plf.get_material_for(sample);
        if (rng.generate() < 1.f -  (mat.Transmission / 1.0f)) {
            result.valid = true;
            result.sample = sample;
            result.position = current_point;
            result.mat = mat;
            break;
        }
    }

    return result;
}

float3 sample_light(const ScatterEvent scatter) {
    return scatter.mat.BaseColor;
}

PLF get_transfer_function() {
    DisneyMaterial air;
    air.Transmission = 1.0f;

    DisneyMaterial tissue;
    tissue.BaseColor = float3(0.6, 0.1, 0.1);
    tissue.Transmission = 0.f;

    PLF plf(air, tissue);

    return plf;
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

    Rng rng;
    PLF plf = get_transfer_function();

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
            ScatterEvent scatter = sample_single_ray(ray, rng, d.volume, plf);
            
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
