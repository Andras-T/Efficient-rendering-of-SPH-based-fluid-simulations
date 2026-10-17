#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace sph::shaders {

// SPIR-V of the module compiled from `<name>.slang`; empty if there is none.
std::span<const std::uint32_t> Find(std::string_view name);

}  // namespace sph::shaders
