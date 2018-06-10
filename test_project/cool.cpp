#include "cool.h"
#include <iostream>

namespace {
    float rand_float() {
        float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
        return r;
    }
}

double dot(std::vector<float> left, std::vector<float> right) {
    double out = 0.0;
    for (int i = 0; i < left.size(); i++) {
        out += left[i] * right[i];
    }
    return out;
}

std::vector<float> do_cool_stuff(int n) {
    std::vector<float> res;
    for (int i = 0; i < n; i++) {
        res.emplace_back(rand_float());
    }
    return res;
}
