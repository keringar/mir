#ifndef DISNEY_H
#define DISNEY_H

#include "math.hpp"

class DisneyMaterial {
public:
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

    DisneyMaterial();

    static DisneyMaterial disney_lerp(DisneyMaterial l, DisneyMaterial r, float t);

    float GetPdf(float3 wi, float3 wo);
    float3 Evaluate(float3 wi, float3 wo);
    float3 Sample(float3 wi, float2 sample, float3& wo, float& pdf);

private:
    float SchlickFresnelReflectance(float u);
    float GTR1(float ndoth, float a);
    float GTR2(float ndoth, float a);
    float GTR2_Aniso(float ndoth, float hdotx, float hdoty, float ax, float ay);
    float SmithGGX_G(float ndotv, float a);
    float SmithGGX_G_Aniso(float ndotv, float vdotx, float vdoty, float ax, float ay);
    float3 Sample_MapToHemisphere(float2 sample, float3 n, float e);
    float3 GetOrthoVector(float3 n);
};

#endif
