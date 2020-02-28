#include "disney.h"

DisneyMaterial::DisneyMaterial() : pad{ 0 } {
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

DisneyMaterial DisneyMaterial::disney_lerp(DisneyMaterial l, DisneyMaterial r, float t) {
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

float DisneyMaterial::GetPdf(float3 wi, float3 wo) {
    float aspect = sqrt(1.f - this->Anisotropy * 0.9f);

    float ax = fmax(0.001f, this->Roughness * this->Roughness * (1.f + this->Anisotropy));
    float ay = fmax(0.001f, this->Roughness * this->Roughness * (1.f - this->Anisotropy));
    float3 wh = normalize(wo + wi);
    float ndotwh = abs(wh.y);
    float hdotwo = abs(dot(wh, wo));

    float d_pdf = abs(wo.y) / PI;
    float r_pdf = GTR2_Aniso(ndotwh, wh.x, wh.z, ax, ay) * ndotwh / (4 * hdotwo);
    float c_pdf = GTR1(ndotwh, lerp(0.1f, 0.001f, this->ClearcoatGloss)) * ndotwh / (4 * hdotwo);

    float3 cd_lin = this->BaseColor;// pow(mat.BaseColor, 2.2);
    // Luminance approximmation
    float cd_lum = dot(cd_lin, float3(0.3f, 0.6f, 0.1f));

    // Normalize lum. to isolate hue+sat
    float3 c_tint = cd_lum > 0 ? (cd_lin / cd_lum) : 1;

    float3 c_spec0 = lerp(this->Specular * 0 * lerp(1, c_tint, this->SpecularTint), cd_lin, this->Metallic);

    float cs_lum = dot(c_spec0, float3(0.3f, 0.6f, 0.1f));

    float cs_w = cs_lum / (cs_lum + (1 - this->Metallic) * cd_lum);

    return c_pdf * this->Clearcoat + (1 - this->Clearcoat) * (cs_w * r_pdf + (1 - cs_w) * d_pdf);
}

float3 DisneyMaterial::Evaluate(float3 wi, float3 wo) {
    float ndotwi = abs(wi.y);
    float ndotwo = abs(wo.y);

    float3 h = normalize(wi + wo);
    float ndoth = abs(h.y);
    float hdotwo = abs(dot(h, wo));

    float3 cd_lin = this->BaseColor;// pow(mat.BaseColor, 2.2);
    // Luminance approximmation
    float cd_lum = dot(cd_lin, float3(0.3f, 0.6f, 0.1f));

    // Normalize lum. to isolate hue+sat
    float3 c_tint = cd_lum > 0 ? (cd_lin / cd_lum) : 1;

    float3 c_spec0 = lerp(this->Specular * 0.1f * lerp(1, c_tint, this->SpecularTint), cd_lin, this->Metallic);

    float3 c_sheen = lerp(1, c_tint, this->SheenTint);

    // Diffuse fresnel - go from 1 at normal incidence to 0.5 at grazing
    // and lerp in diffuse retro-reflection based on Roughness
    float f_wo = SchlickFresnelReflectance(ndotwo);
    float f_wi = SchlickFresnelReflectance(ndotwi);

    float fd90 = 0.5f + 2 * hdotwo * hdotwo * this->Roughness;
    float fd = lerp(1, fd90, f_wo) * lerp(1, fd90, f_wi);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // fss90 used to "flatten" retroreflection based on Roughness
    float fss90 = hdotwo * hdotwo * this->Roughness;
    float fss = lerp(1, fss90, f_wo) * lerp(1, fss90, f_wi);
    float ss = 1.25f * (fss * (1 / (ndotwo + ndotwi) - 0.5f) + 0.5f);

    // Specular
    float ax = fmax(0.001f, this->Roughness * this->Roughness * (1 + this->Anisotropy));
    float ay = fmax(0.001f, this->Roughness * this->Roughness * (1 - this->Anisotropy));
    float ds = GTR2_Aniso(ndoth, h.x, h.z, ax, ay);
    float fh = SchlickFresnelReflectance(hdotwo);
    float3 fs = lerp(c_spec0, 1, fh);

    float gs;
    gs = SmithGGX_G_Aniso(ndotwo, wo.x, wo.z, ax, ay);
    gs *= SmithGGX_G_Aniso(ndotwi, wi.x, wi.z, ax, ay);

    // Sheen
    float3 f_sheen = fh * this->Sheen * c_sheen;

    // Clearcoat (ior = 1.5 -> F0 = 0.04)
    float dr = GTR1(ndoth, lerp(0.1f, 0.001f, this->ClearcoatGloss));
    float fr = lerp(0.04f, 1.f, fh);
    float gr = SmithGGX_G(ndotwo, 0.25) * SmithGGX_G(ndotwi, 0.25);

    return ((1 / PI) * lerp(fd, ss, this->Subsurface) * cd_lin + f_sheen) * (1 - this->Metallic) + gs * fs * ds + this->Clearcoat * gr * fr * dr;
}

float3 DisneyMaterial::Sample(float3 wi, float2 sample, float3& wo, float& pdf) {
    float ax = fmax(0.001f, this->Roughness * this->Roughness * (1 + this->Anisotropy));
    float ay = fmax(0.001f, this->Roughness * this->Roughness * (1 - this->Anisotropy));

    float mis_weight = 1;
    float3 wh;

    if (sample.x < this->Clearcoat) {
        sample.x /= (this->Clearcoat);

        float a = lerp(0.1f, 0.001f, this->ClearcoatGloss);
        float ndotwh = sqrt((1 - pow(a * a, 1 - sample.y)) / (1 - a * a));
        float sintheta = sqrt(1 - ndotwh * ndotwh);
        wh = normalize(float3(cos(2 * PI * sample.x) * sintheta, ndotwh, sin(2 * PI * sample.x) * sintheta));
        wo = -wi + 2 * abs(dot(wi, wh)) * wh;
    } else {
        sample.x -= (this->Clearcoat);
        sample.x /= (1 - this->Clearcoat);

        float3 cd_lin = this->BaseColor;// pow(mat.BaseColor, 2.2);
        // Luminance approximmation
        float cd_lum = dot(cd_lin, float3(0.3f, 0.6f, 0.1f));

        // Normalize lum. to isolate hue+sat
        float3 c_tint = cd_lum > 0.f ? (cd_lin / cd_lum) : 1;

        float3 c_spec0 = lerp(this->Specular * 0.3f * lerp(1, c_tint, this->SpecularTint), cd_lin, this->Metallic);

        float cs_lum = dot(c_spec0, float3(0.3f, 0.6f, 0.1f));
        float cs_w = cs_lum / (cs_lum + (1 - this->Metallic) * cd_lum);

        if (sample.y < cs_w) {
            sample.y /= cs_w;

            float t = sqrt(sample.y / (1 - sample.y));
            wh = normalize(float3(t * ax * cos(2 * PI * sample.x), 1, t * ay * sin(2 * PI * sample.x)));

            wo = -wi + 2.f * abs(dot(wi, wh)) * wh;
        } else {
            sample.y -= cs_w;
            sample.y /= (1.f - cs_w);

            wo = Sample_MapToHemisphere(sample, float3(0, 1, 0), 1);
            wh = normalize(wo + wi);
        }
    }

    //float ndotwh = abs(wh.y);
    //float hdotwo = abs(dot(wh, *wo));

    pdf = GetPdf(wi, wo);
    return Evaluate(wi, wo);
}

float DisneyMaterial::SchlickFresnelReflectance(float u) {
    float m = clamp(1.f - u, 0.f, 1.f);
    float m2 = m * m;
    return m2 * m2 * m;
}

float DisneyMaterial::GTR1(float ndoth, float a) {
    if (a >= 1.f) return 1.f / PI;

    float a2 = a * a;
    float t = 1.f + (a2 - 1.f) * ndoth * ndoth;
    return (a2 - 1.f) / (PI * log(a2) * t);
}

float DisneyMaterial::GTR2(float ndoth, float a) {
    float a2 = a * a;
    float t = 1.f + (a2 - 1.f) * ndoth * ndoth;
    return a2 / (PI * t * t);
}

float DisneyMaterial::GTR2_Aniso(float ndoth, float hdotx, float hdoty, float ax, float ay) {
    float hdotxa2 = (hdotx / ax);
    hdotxa2 *= hdotxa2;
    float hdotya2 = (hdoty / ay);
    hdotya2 *= hdotya2;
    float denom = hdotxa2 + hdotya2 + ndoth * ndoth;
    return denom > 1e-5 ? (1.f / (PI * ax * ay * denom * denom)) : 0.f;
}

float DisneyMaterial::SmithGGX_G(float ndotv, float a) {
    float a2 = a * a;
    float b = ndotv * ndotv;
    return 1.f / (ndotv + sqrt(a2 + b - a2 * b));
}

float DisneyMaterial::SmithGGX_G_Aniso(float ndotv, float vdotx, float vdoty, float ax, float ay) {
    float vdotxax2 = (vdotx * ax) * (vdotx * ax);
    float vdotyay2 = (vdoty * ay) * (vdoty * ay);
    return 1.f / (ndotv + sqrt(vdotxax2 + vdotyay2 + ndotv * ndotv));
}

float3 DisneyMaterial::Sample_MapToHemisphere(float2 sample, float3 n, float e) {
    // Construct basis
    float3 u = GetOrthoVector(n);
    float3 v = cross(u, n);
    u = cross(n, v);

    // Calculate 2D sample
    float r1 = sample.x;
    float r2 = sample.y;

    // Transform to spherical coordinates
    float sinpsi = sin(2 * PI * r1);
    float cospsi = cos(2 * PI * r1);
    float costheta = pow(1.f - r2, 1.f / (e + 1.f));
    float sintheta = sqrt(1.f - costheta * costheta);

    return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta);
}

float3 DisneyMaterial::GetOrthoVector(float3 n) {
    float3 p;
    if (abs(n.z) > 0) {
        float k = sqrt(n.y * n.y + n.z * n.z);
        p.x = 0; p.y = -n.z / k; p.z = n.y / k;
    } else {
        float k = sqrt(n.x * n.x + n.y * n.y);
        p.x = n.y / k; p.y = -n.x / k; p.z = 0;
    }
    return normalize(p);
}
