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
    float3 gradient;
    float3 tangent;
    DisneyMaterial mat;
};

ScatterEvent SampleVolume(const Ray ray, Rng& rng, Volume v, PLF& plf) {
    ScatterEvent result = { 0 };

    // Woodcock (Delta-Tracking?) scattering calculation
    // TODO: Play around with step size, the 0.01 is an arbitrary max step size
    float s = 0.f;
    while (true) {
        s += (-log(1.f - rng.generate()) / 1.0f) * 0.01f;
        
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
            result.gradient = v.gradient_at(current_point);
            result.tangent = v.tangent_at(current_point);
            break;
        }
    }

    return result;
}

float3 shade_surface(Ray& ray, Rng& rng, ScatterEvent& hit, float3& throughput) {
    // Tangent space basis vectors
    float3 normal = hit.gradient;
    float3 tangent = hit.tangent;
    float3 bitangent = cross(normal, tangent);
    
    // Evaluate the BRDF
    float3 wo;
    float probability;
    float3 wi = -ray.direction;
    float3 wi_t = float3(tangent.x * wi.x + tangent.y * wi.y + tangent.z * wi.z,
                         normal.x * wi.x + normal.y * wi.y + normal.z * wi.z,
                         bitangent.x * wi.x + bitangent.y * wi.y + bitangent.z * wi.z);
    float3 brdf = hit.mat.Sample(wi_t, float2(rng.generate(), rng.generate()), wo, probability);

    // If material is an emitter, then it's light source and we can stop bouncing
    if (hit.mat.Emission.x > 0 || hit.mat.Emission.y > 0 || hit.mat.Emission.z > 0) {
        float3 radiance = throughput * hit.mat.Emission; // * weight;
        throughput = 0.f;

        return radiance;
    }

    float3 radiance = 0.f;

    // Send a ray to a random light in the scene (handle direct lighting)
    if (false) { // If # lights > 0
        // then sample a random light
        // and set the radiance to the light and material properties
    }

    // Next bounce (for additional indirect lighting)
    ray.origin = hit.position;
    ray.direction = normalize(float3(tangent.x * wo.x + normal.x * wo.y + bitangent.x * wo.z,
                                     tangent.y * wo.x + normal.y * wo.y + bitangent.y * wo.z,
                                     tangent.z * wo.x + normal.z * wo.y + bitangent.z * wo.z));

    // Keep track of the evalulated material until we hit a light source
    throughput *= abs(dot(normal, ray.direction)) * brdf / probability;

    // If we didn't hit a light, radiance is zero. Otherwise, returns the radiance from the light
    return clamp(radiance, 0, 3);
}

float3 trace_ray(Ray ray, Rng& rng, Volume volume, PLF plf) {
    // Scene intersection
    ScatterEvent hit = SampleVolume(ray, rng, volume, plf);
    if (!hit.valid) {
        return 0;
    }

    // Shade the surface that was hit
    float3 radiance = shade_surface(ray, rng, hit, throughput);
}

PLF get_transfer_function() {
    DisneyMaterial air;
    air.Transmission = 1.0f;

    DisneyMaterial tissue;
    tissue.BaseColor = float3(0.6f, 0.1f, 0.1f);
    tissue.Transmission = 0.2f;

    DisneyMaterial bone;
    bone.BaseColor = float3(1.0, 1.0, 1.0);
    bone.Transmission = 0.f;

    PLF plf(air, tissue);
    plf.add_material(6000, air);
    plf.add_material(6001, bone);
    plf.add_material(7000, bone);
    plf.add_material(7001, tissue);

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
            // Generate ray
            const float3 eye = float3(0.f, 0.f, -1.f);
            const float target_x = ((2.f * (float)x / (float)OUTPUT_WIDTH) - 1.f);
            const float target_y = ((2.f * (float)y / (float)OUTPUT_HEIGHT) - 1.f);
            const float3 target = float3(target_x, target_y, eye.z + 4.0f);
            const float3 direction = normalize(target - eye);
            Ray ray = { direction, eye };

            float3 hdr_color = trace_ray(ray, rng, d.volume, plf);

            float3 ldr = tonemap_aces(hdr_color);
            image[x + (y * OUTPUT_HEIGHT)] = pow(ldr, 2.2f);
        }
    }

    cout << "Writing output image to HDR file: " << argv[2] << endl;
    stbi_write_hdr(argv[2], OUTPUT_WIDTH, OUTPUT_HEIGHT, 3, (float*)image);
    delete[] image;

    return 0;
}
