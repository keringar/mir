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
#define NUM_BOUNCES 16
#define SAMPLES_PER_PIXEL 1000

struct Ray {
    float3 direction;
    float3 origin;
};

struct ScatterEvent {
    bool valid;
    float distance;
    uint16_t sample;
    float3 position;
    float3 gradient;
    float3 tangent;
    DisneyMaterial mat;
};

ScatterEvent SampleVolume(const Ray ray, Rng& rng, Volume v, PLF& plf) {
    ScatterEvent result = { 0 };
    result.valid = false;
    result.distance = 0.f;

    while (result.distance < 5.f) {
        // TODO: Make this distance tracking better
        result.distance += clamp(-log(1.f - rng.generate()) / 1.0f, 0.f, 5.f) * 0.01f;

        // Check if out of bounds
        float3 current_point = ray.origin + ray.direction * result.distance;
        if (abs(current_point.x) > 5.f || abs(current_point.y) > 5.f || abs(current_point.z) > 5.f) {
            break;
        }

        // Check if we hit something
        if (abs(dot(current_point, current_point)) <= 0.05f) {
            result.valid = true;
            result.position = current_point;
            result.sample = 100;
            result.mat = plf.get_material_for(result.sample);
            result.gradient = normalize(current_point);
            result.tangent = normalize(float3(result.gradient.z, result.gradient.z, -result.gradient.x - result.gradient.y));

            break;
        }
    }

    return result;
}

float3 SampleLights(float3 wi_t, float3 p, float3 n, DisneyMaterial material) {
    float3 radiance = 0.f;

    // Sample every light in the scene
    for (size_t i = 0; i < 1; i++) {
        // Just one simple point light for now
        // TODO: Figure out why light position is flipped over the x-z plane
        const float3 LIGHT_POS = float3(1.0, -1.0, 1.0);
        float3 wo = normalize(LIGHT_POS - p);

        // Tangent space basis vectors
        float3 normal = n;
        float3 tangent = normalize(float3(normal.z, normal.z, -normal.x - normal.y));
        float3 bitangent = cross(tangent, normal);

        // Convert outgoing angle to tangent space
        float3 wo_t = float3(tangent.x * wo.x + tangent.y * wo.y + tangent.z * wo.z,
            normal.x * wo.x + normal.y * wo.y + normal.z * wo.z,
            bitangent.x * wo.x + bitangent.y * wo.y + bitangent.z * wo.z);
        
        // Multiply light contribution by light emissive color
        radiance += material.Evaluate(wi_t, wo_t) * float3(1.f);
    }

    return radiance;
}

float3 trace_ray(Ray ray, Rng& rng, Volume volume, PLF plf) {
    // TODO: handle intersecting the actual light itself. Right now, all lights
    // will show up as black if the ray intersected it.

    float3 color;
    float3 throughput = float3(1.f);

    for (int i = 0; i < NUM_BOUNCES; i++) {
        ScatterEvent hit = SampleVolume(ray, rng, volume, plf);

        // The ray missed
        if (!hit.valid) {
            color += throughput * float3(0.f); // add background color
            break;
        }

        DisneyMaterial material = hit.mat;
        
        // Check if material is emissive
        if (material.Emission.r > 0 || material.Emission.g > 0 || material.Emission.b > 0) {
            color += throughput * material.Emission;
        }

        // Tangent space basis vectors
        float3 normal = hit.gradient;
        float3 tangent = hit.tangent;
        float3 bitangent = cross(tangent, normal);

        // Sample the material BSDF
        float3 wo_t;
        float pdf;
        float3 wi = -ray.direction;
        float3 wi_t = float3(tangent.x * wi.x + tangent.y * wi.y + tangent.z * wi.z, // Convert normal to tangent space
                             normal.x * wi.x + normal.y * wi.y + normal.z * wi.z,
                             bitangent.x * wi.x + bitangent.y * wi.y + bitangent.z * wi.z);
        float3 brdf = material.Sample(wi_t, float2(rng.generate(), rng.generate()), wo_t, pdf);

        // Calculate direct lighting
        color += throughput * SampleLights(wi_t, hit.position, hit.gradient, hit.mat);

        // Accumulate the weighted brdf
        throughput *= material.Evaluate(wi_t, wo_t) / pdf;

        // Convert output direction back to world coords
        float3 wo = float3(tangent.x * wo_t.x + normal.x * wo_t.y + bitangent.x * wo_t.z,
                           tangent.y * wo_t.x + normal.y * wo_t.y + bitangent.y * wo_t.z,
                           tangent.z * wo_t.x + normal.z * wo_t.y + bitangent.z * wo_t.z);
    
        // Find the next bounce direction
        ray.origin = hit.position;
        ray.direction = normalize(wo);
    }

    return color;
}

PLF get_transfer_function() {
    DisneyMaterial air;
    air.Transmission = 1.0f;

    DisneyMaterial tissue;
    tissue.BaseColor = float3(0.6f, 0.1f, 0.1f);
    tissue.Metallic = 0.0f;
    tissue.Emission = float3(0.f);
    tissue.Specular = 0.f;
    tissue.Anisotropy = 0.f;
    tissue.Roughness = 0.8f;
    tissue.SpecularTint = 0.1f;
    tissue.SheenTint = 0.0f;
    tissue.Sheen = 0.0f;
    tissue.ClearcoatGloss = 0.1f;
    tissue.Clearcoat = 0.1f;
    tissue.Subsurface = 0.0f;
    tissue.Transmission = 0.f;

    PLF plf(air, tissue);
    plf.add_material(1, air);
    plf.add_material(2, tissue);

    return plf;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
float3 tonemap_aces(float3 hdr_color) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((hdr_color * (a * hdr_color + b)) / (hdr_color * (c * hdr_color + d) + e), float3(0.f), float3(1.f));
}

int main(int argc, char** argv) {
    argc = 3;
    argv[1] = "..\\..\\..\\data\\";
    argv[2] = ".\\out.hdr";

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
    for (uint32_t y = 0; y < OUTPUT_HEIGHT; y++) {
        for (uint32_t x = 0; x < OUTPUT_WIDTH; x++) {
            // Generate ray
            const float3 eye = float3(0.f, 0.f, -1.f);
            const float target_x = ((2.f * (float)x / (float)OUTPUT_WIDTH) - 1.f);
            const float target_y = ((2.f * (float)y / (float)OUTPUT_HEIGHT) - 1.f);
            const float3 target = float3(target_x, target_y, eye.z + 4.0f);
            const float3 direction = normalize(target - eye);
            Ray ray = { direction, eye };

            cout << "\rTracing ray " << x + OUTPUT_WIDTH * y << " of " << OUTPUT_WIDTH * OUTPUT_HEIGHT << flush;
            float3 hdr_color = 0;
            for (size_t i = 0; i < SAMPLES_PER_PIXEL; i++) {
                hdr_color = trace_ray(ray, rng, d.volume, plf);
            }
            hdr_color /= (float)SAMPLES_PER_PIXEL;

            float3 ldr = tonemap_aces(hdr_color);
            image[x + (y * OUTPUT_HEIGHT)] = pow(ldr, 2.2f);
        }
    }

    cout << "\nWriting output image to HDR file: " << argv[2] << endl;
    stbi_write_hdr(argv[2], OUTPUT_WIDTH, OUTPUT_HEIGHT, 3, (float*)image);
    delete[] image;

    return 0;
}
