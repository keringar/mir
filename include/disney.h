#ifndef DISNEY_H
#define DISNEY_H

#include "math.hpp"

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

    DisneyMaterial() : pad {0} {
        BaseColor = float3(0);
        Metallic = 0.f;
        Emission = float3(0);
        Specular = 0.f;
        Anisotropy = 0.f;
        Roughness = 0.f;
        SpecularTint = 0.f;
        SheenTint = 0.f;
        Sheen = 0.f;
        ClearcoatGloss = 0.f;
        Clearcoat = 0.f;
        Transmission = 1.f;
    }

    static DisneyMaterial disney_lerp(DisneyMaterial l, DisneyMaterial r, float t) {
        DisneyMaterial mixed;
        mixed.BaseColor = lerp(l.BaseColor, r.BaseColor, t);
        mixed.Metallic = lerp(l.Metallic, r.Metallic, t);
        mixed.Emission = lerp(l.Emission, r.Emission, t);
        mixed.Specular = lerp(l.Specular, r.Specular, t);
        mixed.Anisotropy = lerp(l.Anisotropy, r.Anisotropy, t);
        mixed.Roughness = lerp(l.Roughness, r.Roughness, t);
        mixed.SpecularTint = lerp(l.SpecularTint, r.SpecularTint, t);
        mixed.SheenTint = lerp(l.SheenTint, r.SheenTint, t);
        mixed.Sheen = lerp(l.Sheen, r.Sheen, t);
        mixed.ClearcoatGloss = lerp(l.ClearcoatGloss, r.ClearcoatGloss, t);
        mixed.Clearcoat = lerp(l.Clearcoat, r.Clearcoat, t);
        mixed.Subsurface = lerp(l.Subsurface, r.Subsurface, t);
        mixed.Transmission = lerp(l.Transmission, r.Transmission, t);

        return mixed;
    }
};

#endif
