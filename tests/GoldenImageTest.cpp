#include "app/Headless.hpp"
#include "app/ImageWriter.hpp"
#include "gpu/Context.hpp"
#include "render/Camera.hpp"
#include "render/DebugView.hpp"
#include "sim/Scene.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <string_view>

#ifndef SPH_GOLDEN_DIR
#define SPH_GOLDEN_DIR ""
#endif

namespace sph {
namespace {

constexpr VkExtent2D GoldenExtent{96, 54};

std::filesystem::path GoldenPath(std::string_view name) {
  return std::filesystem::path(SPH_GOLDEN_DIR) / name;
}

sim::Scene CompactFluid() {
  sim::Scene scene;
  scene.params.particleSpacing = 0.08F;
  scene.params.domainMin = {-0.35F, 0.0F, -0.25F};
  scene.params.domainMax = {0.35F, 0.55F, 0.25F};
  const float spacing = scene.params.particleSpacing;
  for (int z = 0; z < 4; ++z) {
    for (int y = 0; y < 5; ++y) {
      for (int x = 0; x < 4; ++x) {
        sim::Particle particle;
        particle.position = {(-1.5F + static_cast<float>(x)) * spacing,
                             (0.5F + static_cast<float>(y)) * spacing,
                             (-1.5F + static_cast<float>(z)) * spacing};
        particle.mass = scene.params.mass();
        particle.density = scene.params.restDensity;
        scene.particles.push_back(particle);
      }
    }
  }
  return scene;
}

render::Camera GoldenCamera(const sim::Scene& scene) {
  render::Camera camera;
  camera.setAspect(GoldenExtent);
  camera.frame(scene.params.domainMin, scene.params.domainMax);
  camera.setYawPitch(0.55F, -0.32F);
  camera.setDistance(1.55F);
  return camera;
}

double MeanAbsError(const std::vector<std::uint8_t>& actual,
                    const std::vector<std::uint8_t>& expected) {
  EXPECT_EQ(actual.size(), expected.size());
  if (actual.size() != expected.size() || actual.empty()) {
    return 1.0e9;
  }
  double sum = 0.0;
  int maxDiff = 0;
  for (std::size_t i = 0; i < actual.size(); ++i) {
    const int diff = std::abs(static_cast<int>(actual[i]) - static_cast<int>(expected[i]));
    sum += diff;
    maxDiff = std::max(maxDiff, diff);
  }
  EXPECT_LE(maxDiff, 40);
  return sum / static_cast<double>(actual.size());
}

void CompareGolden(const gpu::Context& context, std::string_view name,
                   const OffscreenRequest& request) {
  const auto rgba = RenderOffscreen(context, request);
  ASSERT_EQ(rgba.size(), std::size_t{request.extent.width} * request.extent.height * 4);
  const auto path = GoldenPath(name);
#ifdef SPH_WRITE_GOLDENS
  std::filesystem::create_directories(path.parent_path());
  WritePng(path, request.extent, rgba);
#endif
  ASSERT_TRUE(std::filesystem::exists(path)) << path.string();
  VkExtent2D loaded{};
  const auto expected = ReadPng(path, loaded);
  EXPECT_EQ(loaded.width, request.extent.width);
  EXPECT_EQ(loaded.height, request.extent.height);
  EXPECT_LT(MeanAbsError(rgba, expected), 6.0);
}

TEST(GoldenImageTest, SkyAndFluidStagesMatchLavapipe) {
  const gpu::Context context({.validation = true});
  const sim::Scene empty;
  render::Camera skyCamera;
  skyCamera.setAspect(GoldenExtent);
  skyCamera.setYawPitch(0.35F, -0.15F);
  skyCamera.setDistance(2.4F);

  CompareGolden(context, "sky.png",
                {.extent = GoldenExtent, .preset = sim::ScenePreset::Empty, .camera = &skyCamera});

  const sim::Scene fluid = CompactFluid();
  const render::Camera camera = GoldenCamera(fluid);
  CompareGolden(context, "fluid-final.png",
                {.extent = GoldenExtent,
                 .scene = &fluid,
                 .camera = &camera,
                 .view = render::DebugView::Final});
  CompareGolden(context, "fluid-depth.png",
                {.extent = GoldenExtent,
                 .scene = &fluid,
                 .camera = &camera,
                 .view = render::DebugView::SmoothDepth});
  CompareGolden(context, "fluid-normals.png",
                {.extent = GoldenExtent,
                 .scene = &fluid,
                 .camera = &camera,
                 .view = render::DebugView::Normals});
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
