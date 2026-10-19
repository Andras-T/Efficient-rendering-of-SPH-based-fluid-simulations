#pragma once

#include "gpu/Timestamps.hpp"
#include "gpu/Vulkan.hpp"
#include "render/Camera.hpp"

#include <span>
#include <string_view>

namespace sph::ui {

void ApplyCameraInput(render::Camera& camera);

void DrawHud(std::string_view deviceName, VkExtent2D extent, render::Camera& camera,
             std::span<const gpu::GpuProfiler::Interval> timings);

}  // namespace sph::ui
