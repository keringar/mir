#pragma once

#include <random>

class Rng {
private:
        std::mt19937 rng;
        std::uniform_real_distribution<> dis;

public:

    Rng();

    // Generates a random float between [0,1)
    float generate();
};
