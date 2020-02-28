#include "plf.h"

PLF::PLF(DisneyMaterial first, DisneyMaterial second) {
    add_material(0, first);
    add_material(65535, second);
}

void PLF::add_material(uint16_t value, DisneyMaterial node) {
    if (nodes.empty()) {
        nodes.emplace(nodes.begin(), value, node);
        return;
    }

    for (int i = 0; i < nodes.size(); i++) {
        if (std::get<0>(nodes[i]) > value) {
            nodes.emplace(nodes.begin() + i, value, node);
            return;
        } else if (std::get<0>(nodes[i]) == value) {
            nodes[i] = std::make_pair(value, node);
            return;
        }
    }

    nodes.emplace_back(value, node);
}

DisneyMaterial PLF::get_material_for(uint16_t sample) {
    int upper_i = nodes.size() - 1;
    for (int i = 0; i < nodes.size() - 1; i++) {
        uint16_t value = std::get<0>(nodes[i]);
        if (value == sample) {
            return std::get<1>(nodes[i]);
        } else if (value > sample) {
            upper_i = i;
            break;
        }
    }

    uint16_t sample_left = std::get<0>(nodes[upper_i - 1]);
    uint16_t sample_right = std::get<0>(nodes[upper_i]);
    float mix_percentage = (float)(sample - sample_left) / (float)(sample_right - sample_left);

    DisneyMaterial mat_l = std::get<1>(nodes[upper_i - 1]);
    DisneyMaterial mat_r = std::get<1>(nodes[upper_i]);
    return DisneyMaterial::disney_lerp(mat_l, mat_r, mix_percentage);
}
