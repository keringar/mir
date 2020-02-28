#include "rng.h"

Rng::Rng() {
    std::random_device rd;
    this->rng = std::mt19937(rd());
}

float Rng::generate() {
    return dis(rng);
}
