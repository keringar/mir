#ifndef PLF_H
#define PLF_H

#include "disney.h"

#include <vector>
#include <utility>

class PLF {
private:
    // TODO: btree opt
    std::vector<std::pair<uint16_t, DisneyMaterial>> nodes;

public:
    PLF(DisneyMaterial first, DisneyMaterial second);
    void add_material(uint16_t value, DisneyMaterial node);

    DisneyMaterial get_material_for(uint16_t sample);
};

#endif
