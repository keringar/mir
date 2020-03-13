#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Dicom.hpp"
#include "math.hpp"
#include "disney.h"
#include "rng.h"
#include "plf.h"
#include "filesystem.hpp"
#include "camera.h"
#include "ray.h"

#include <iostream>
#include <cmath>

using namespace std;

#define OUTPUT_WIDTH 1024
#define OUTPUT_HEIGHT 1024
#define NUM_BOUNCES 1
#define SAMPLES_PER_PIXEL 1000
#define MAX_DENSITY 1.f // affects the chance of check for a hit being true
#define DENSITY_MULTIPLIER 100.f // increases probability of checking for a hit

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

    /* Ray Marching */
    while (result.distance < 2.f) {
        result.distance += 0.001f;

        // Check if out of bounds
        float3 current_point = ray.origin + ray.direction * result.distance;
        if (abs(current_point.x) > 2.f || abs(current_point.y) > 2.f || abs(current_point.z) > 2.f) {
            break;
        }

        // Check if we hit something
        uint32_t sample = v.sample_at(current_point);
        DisneyMaterial mat = plf.get_material_for(sample);
        if (mat.Transmission < 1.f) { // TODO: handle volumetric scattering
            result.valid = true;
            result.position = current_point;
            result.sample = sample;
            result.mat = mat;
            result.gradient = v.gradient_at(current_point);
            result.tangent = normalize(float3(result.gradient.z, result.gradient.z, -result.gradient.x - result.gradient.y));

            break;
        }
    }

    /* Delta tracking for volumetric scattering */
    /*
    while (result.distance < 1.f) {
        result.distance -= logf(1.f - rng.generate()) / (MAX_DENSITY * DENSITY_MULTIPLIER);

        float3 current_point = ray.origin + ray.direction * result.distance;
        uint32_t sample = v.sample_at(current_point);
        DisneyMaterial mat = plf.get_material_for(sample);

        float density = 1.f - mat.Transmission;
        if ((density / MAX_DENSITY) > rng.generate()) {
            result.valid = true;
            result.position = current_point;
            result.sample = sample;
            result.mat = mat;
            result.gradient = v.gradient_at(current_point);
            result.tangent = normalize(float3(result.gradient.z, result.gradient.z, -result.gradient.x - result.gradient.y));
            
            break;
        }
    }
    */

    return result;
}

float3 SampleLights(float3 wi_t, float3 p, float3 n, DisneyMaterial material, Rng& rng, Volume v, PLF& plf) {
    float3 radiance = 0.f;

    // Sample every light in the scene
    for (size_t i = 0; i < 1; i++) {
        // Just one simple point light for now
        // TODO: Figure out why light position is flipped over the x-z plane
        const float3 LIGHT_POS = float3(0.0, -1.0, 0.0);
        float3 wo = normalize(LIGHT_POS - p);

        // Check if light ray is reachable
        Ray light_ray;
        light_ray.origin = p;
        light_ray.direction = wo;
        ScatterEvent light_hit = SampleVolume(light_ray, rng, v, plf);
        if (light_hit.distance >= length(LIGHT_POS - p)) {
            // Sample ray did not reach the light
            continue;
        }

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
        float3 radiance = SampleLights(wi_t, hit.position, hit.gradient, hit.mat, rng, volume, plf);
        color += throughput * radiance;

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

// TODO: Integrate into transfer function editor? Sooo slow to iterate when I do this by hand
PLF get_transfer_function() {
    DisneyMaterial one;
    one.Transmission = 1.0f;
    one.BaseColor = float3(0.62f, 0.62f, 0.64f);
    one.Specular = 0.03f;
    one.Emission = float3(0.f);
    one.Roughness = 1.f;

    DisneyMaterial two;
    two.Transmission = 1.0f;
    two.BaseColor = float3(0.17f, 0.f, 0.f);
    two.Specular = 0.66f;
    two.Emission = float3(0.f);
    two.Roughness = 0.f;

    DisneyMaterial three;
    three.Transmission = 0.982972f;
    three.BaseColor = float3(0.17f, 0.02f, 0.02f);
    three.Specular = 0.19f;
    three.Emission = float3(0.f);
    three.Roughness = 0.5868f;

    DisneyMaterial four;
    four.Transmission = 0.436498f;
    four.BaseColor = float3(0.26f, 0.f, 0.f);
    four.Specular = 0.027f;
    four.Emission = float3(0.f);
    four.Roughness = 0.0744f;

    DisneyMaterial five;
    five.Transmission = 0.0f;
    five.BaseColor = float3(0.337f, 0.282f, 0.16f);
    five.Specular = 0.0078f;
    five.Emission = float3(0.f);
    five.Roughness = 0.1322f;

    DisneyMaterial six;
    six.Transmission = 0.0f;
    six.BaseColor = float3(0.62745f, 0.62745f, 0.64313f);
    six.Specular = 0.039f;
    six.Emission = float3(0.f);
    six.Roughness = 1.f;

    PLF plf(one, six);
    plf.add_material(15475, two);
    plf.add_material(18909, three);
    plf.add_material(18911, four);
    plf.add_material(23200, five);

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
    if (argc != 3) {
        cout << "Usage: mir <data folder> <output filename>" << endl;
        return 0;
    }

    float3 size(1.f, 1.f, 1.f);
    Dicom d;
    if (d.LoadDicomStack(argv[1], &size, false, 1)) {
        cerr << "FATAL: Error loading Dicom stack" << endl;
        return -1;
    } else {
        cout << "Successfully loaded DCM stack" << endl;
    }

    cout << "Maximum Sample Value " << d.max_value << endl;

    Rng rng;
    PLF plf = get_transfer_function();

    Camera camera = Camera(float3(0.3f, 0.4f, 0.3f), float3(0, 0, 0), float3(0, 0, 1), OUTPUT_WIDTH, OUTPUT_HEIGHT);

    cout << "Raytracing " << OUTPUT_WIDTH << "x" << OUTPUT_HEIGHT << " image" << endl;
    float3* image = new float3[OUTPUT_WIDTH * OUTPUT_HEIGHT];
    for (uint32_t y = 0; y < OUTPUT_HEIGHT; y++) {
        for (uint32_t x = 0; x < OUTPUT_WIDTH; x++) {
            cout << "\rTracing ray " << x + OUTPUT_WIDTH * y << " of " << OUTPUT_WIDTH * OUTPUT_HEIGHT << flush;

            // Sample pixel at x,y
            float3 hdr_color = 0;
            for (size_t i = 0; i < SAMPLES_PER_PIXEL; i++) {
                Ray ray = camera.get_ray(x, y, true, rng);
                hdr_color += trace_ray(ray, rng, d.volume, plf);
            }
            hdr_color /= SAMPLES_PER_PIXEL;

            // Tonemap and gamma correct
            float3 ldr = tonemap_aces(hdr_color);
            image[x + (y * OUTPUT_HEIGHT)] = pow(ldr, 2.2f);
        }
    }

    cout << "\nWriting output image to file: " << argv[2] << endl;
    stbi_write_hdr(argv[2], OUTPUT_WIDTH, OUTPUT_HEIGHT, 3, (float*)image);
    delete[] image;

    return 0;
}
