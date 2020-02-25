#pragma once

#include "math.hpp"
#include <string>

class Dicom {
    public:
        uint16_t* data;
        uint32_t width, height, depth;

        Dicom();
        ~Dicom();

        int LoadDicomStack(const std::string& folder, float3* size);
};
