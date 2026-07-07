#pragma once
#include <random>

namespace rng {
void         seed(unsigned s);
std::mt19937& engine();
} // namespace rng
