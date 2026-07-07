#include "rng.h"

namespace rng {
namespace {
std::mt19937 s_engine; // NOLINT(cert-msc51-cpp) — seeded explicitly via rng::seed
} // namespace

void seed(unsigned s) { s_engine.seed(s); }

std::mt19937& engine() { return s_engine; }
} // namespace rng
