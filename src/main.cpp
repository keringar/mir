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

#define OUTPUT_WIDTH 128
#define OUTPUT_HEIGHT 128

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

    for (int i = 0; i < 100; i++) {
        result.distance += 0.01f;
        
        float3 current_point = ray.origin + ray.direction * result.distance;
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


    /*

    ScatterEvent result = { 0 };
    result.valid = false;

    // Woodcock (Delta-Tracking?) scattering calculation
    // TODO: Play around with step size, the 0.01 is an arbitrary max step size
    float s = 0.f;
    while (s < 1000) {
        s += clamp(-log(1.f - rng.generate()) / 1.0f, 0.f, 5.f) * 0.01f;
        
        float3 current_point = ray.origin + (s * ray.direction);
        if (abs(current_point.x) > MAX_BOUNDS || abs(current_point.y) > MAX_BOUNDS || abs(current_point.z) > MAX_BOUNDS) {
            break;
        }

        uint16_t sample = v.sample_at(current_point);
        DisneyMaterial mat = plf.get_material_for(sample);
        if (rng.generate() < 1.f - (mat.Transmission / 1.0f)) {
            result.valid = true;
            result.sample = sample;
            result.position = current_point;
            result.mat = mat;
            result.gradient = v.gradient_at(current_point);
            result.tangent = v.tangent_at(current_point);
            break;
        }
    }

    result.distance = s;
    return result;
    */
}

float BalanceHeuristic(int nf, float fpdf, int ng, float gpdf) {
    float f = nf * fpdf;
    float g = ng * gpdf;
    return f / (f + g);
}

float3 sampleAreaLight(float2 sample, float3 p, float3 n, float3& wo, float& pdf) {
    float3 light_pos = float3((sample.x * 1.f) - 2.f, -2.f, (sample.y * 1.f) - 2.f);
    float3 light_normal = float3(0.f, -1.f, 0.f);
    float3 light_tangent = normalize(float3(light_normal.z, light_normal.z, -light_normal.x - light_normal.y));
    float3 light_bitangent = normalize(cross(light_tangent, light_normal));

    float sinThetaMax2 = light.radius * light.radius / distanceSq(light.position, interaction.point);
    float cosThetaMax = sqrt(max(EPSILON, 1. - sinThetaMax2));
    wi = uniformSampleCone(u, cosThetaMax, tangent, binormal, lightDir);

    if (dot(wi, n) > 0.) {
        pdf = 1.f / (2.f * PI * (1.f - cosThetaMax));
    }

    return light.L;

    /*
    // TODO: add more lights
    // TODO: figure out why the lighting coords are flipped over the x-z plane
    float3 light_pos = float3((sample.x * 1.f) - 2.f, -2.f, (sample.y * 1.f) - 2.f);
    float3 light_normal = float3(0.f, -1.f, 0.f);

    wo = light_pos - p;
    float nv = dot(n, normalize(wo));
    if (nv <= 0) {
        pdf = 0;
        return 0;
    }

    float3 ke = 1.0f; // TODO: Replace with light material emission

    float d2 = dot(wo, wo);
    float d = (nv * 0.5f * light_normal).x; // TODO: why do we only consider the first component?
    pdf = d > 0 ? d2 / d : 0;
    return d2 > 0 ? ke * nv / d2 : 0;
    */
}

float3 shade_surface(Ray& ray, Rng& rng, ScatterEvent& hit, float3& throughput, Volume& volume, PLF& plf) {
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
        float denom = (abs(dot(normal, wi)) * 0.5f * normal).x; // TODO: why do we only consider the first component?
        float bxdflightpdf = denom > 0 ? (hit.distance * hit.distance / (denom * 1)) : 0;
        float weight = BalanceHeuristic(1, probability, 1, bxdflightpdf);

        float3 radiance = throughput * hit.mat.Emission * weight;
        throughput = 0.f;
        return radiance;
    }

    float3 radiance = 0.f;
    // Send a ray to a random light in the scene (handle direct lighting)
    if (true) {
        float lightpdf;
        float3 lwo;
        float3 le = sampleAreaLight(float2(rng.generate(), rng.generate()), hit.position, normal, lwo, lightpdf);

        float light_dist = sqrt(abs(dot(lwo, lwo)));

        Ray light_ray;
        light_ray.origin = hit.position;
        light_ray.direction = normalize(lwo);
        
        ScatterEvent light_hit = SampleVolume(light_ray, rng, volume, plf);
        if (!light_hit.valid || light_hit.distance >= light_dist) {
            float weight = BalanceHeuristic(1, probability, 1, lightpdf);
            lwo = normalize(lwo);
            float nwo = abs(dot(lwo, normal));
            float3 material_eval = hit.mat.Evaluate(wi, lwo);
            radiance = le * nwo * material_eval * throughput * weight;
        }
    }

    // Next bounce (for additional indirect lighting)
    ray.origin = hit.position;
    ray.direction = normalize(float3(tangent.x * wo.x + normal.x * wo.y + bitangent.x * wo.z,
                                     tangent.y * wo.x + normal.y * wo.y + bitangent.y * wo.z,
                                     tangent.z * wo.x + normal.z * wo.y + bitangent.z * wo.z));

    // Keep track of the evalulated materials we've hit so far
    throughput *= abs(dot(normal, ray.direction)) * brdf / probability;

    // If we didn't hit a light, radiance is zero. Otherwise, returns the radiance from the light
    return clamp(radiance, 0, 3);
}

float3 trace_ray(Ray ray, Rng& rng, Volume volume, PLF plf) {
    // Scene intersection one
    float3 primary;
    for (int i = 0; i < 16; i++) {
        float3 throughput = float3(1.f);

        ScatterEvent hit = SampleVolume(ray, rng, volume, plf);
        if (hit.valid) {
            primary += shade_surface(ray, rng, hit, throughput, volume, plf);
        }
    }

    // TODO: secondary rays
    return primary;
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
            hdr_color = trace_ray(ray, rng, d.volume, plf);

            float3 ldr = tonemap_aces(hdr_color);
            image[x + (y * OUTPUT_HEIGHT)] = pow(ldr, 2.2f);
        }
    }

    cout << "\nWriting output image to HDR file: " << argv[2] << endl;
    stbi_write_hdr(argv[2], OUTPUT_WIDTH, OUTPUT_HEIGHT, 3, (float*)image);
    delete[] image;

    return 0;
}
