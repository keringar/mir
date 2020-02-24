#include "Dicom.hpp"
#include "math.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>

using namespace std;

#define OUTPUT_WIDTH 512
#define OUTPUT_HEIGHT 512

uint16_t sample_at(uint16_t* data, uint32_t width, uint32_t height, uint32_t depth, float3 size, float3 world_pos) {


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

    cout << "Raytracing " << OUTPUT_WIDTH << "x" << OUTPUT_HEIGHT << " image" << endl;
    float3* image = new float3[OUTPUT_WIDTH * OUTPUT_HEIGHT];
    for (uint32_t x = 0; x < OUTPUT_WIDTH; x++) {
        for (uint32_t y = 0; y < OUTPUT_HEIGHT; y++) {
            const float3 eye = float3(0.f, 0.f, -5.f);
            const float3 target = float3(x - (float)(d.width / 2), y - (float)(d.height / 2), 0.0);
            const float3 direction = normalize(target - eye);

            // Ray trace
            float distance = 0.f;
            float3 sampled_color = 0.f;
            for (int i = 0; i < 1000; i++) {
                distance += 0.01f;
                float3 point = eye + (direction * distance);

                if (sample_at(d.data, d.width, d.height, d.depth, size, point) > 0) {
                    // Hit
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
