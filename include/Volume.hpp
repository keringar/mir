#pragma once

#include "math.hpp"
#include <vector>

class Volume {
private:
    std::vector<uint16_t> data;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_depth;
    float3 m_size;

public:
    Volume();
    Volume(uint32_t width, uint32_t height, uint32_t depth);

    uint32_t width() { return m_width; }
    uint32_t height() { return m_height; }
    uint32_t depth() { return m_depth; }
    float3 size() { return m_size; }
    uint16_t* get_raw_data();

    void set_size(float3 size);

    uint16_t sample_at(float3 world_pos);
    float3 gradient_at(float3 world_pos);
    float3 tangent_at(float3 world_pos);
};

