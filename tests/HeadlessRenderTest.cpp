#include "app/Headless.hpp"
#include "app/ImageWriter.hpp"
#include "core/Capture.hpp"
#include "gpu/Context.hpp"
#include "render/Camera.hpp"
#include "sim/Scene.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>

namespace sph {
namespace {

struct Pixel {
  int r;
  int g;
  int b;
};

Pixel PixelAt(const std::vector<std::uint8_t>& rgba, VkExtent2D extent, std::uint32_t x,
              std::uint32_t y) {
  const std::size_t offset = ((std::size_t{y} * extent.width) + x) * 4;
  return {rgba.at(offset), rgba.at(offset + 1), rgba.at(offset + 2)};
}

TEST(HeadlessRenderTest, SkyIsLitHdrEnvironment) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{64, 48};
  const auto rgba = RenderOffscreen(context, Extent, 2);
  ASSERT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);

  const Pixel top = PixelAt(rgba, Extent, Extent.width / 2, 4);
  const Pixel bottom = PixelAt(rgba, Extent, Extent.width / 2, Extent.height - 5);
  const Pixel left = PixelAt(rgba, Extent, 2, Extent.height / 2);
  const Pixel right = PixelAt(rgba, Extent, Extent.width - 3, Extent.height / 2);
  const int topLum = top.r + top.g + top.b;
  const int bottomLum = bottom.r + bottom.g + bottom.b;
  EXPECT_GT(topLum, 30);
  EXPECT_GT(bottomLum, 20);
  EXPECT_GT(std::abs(left.r - right.r) + std::abs(left.g - right.g) + std::abs(left.b - right.b),
            8);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, SequenceWritesEveryFrame) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{32, 24};
  const auto directory = std::filesystem::temp_directory_path() / "sph-fluid-sequence-test";
  std::filesystem::remove_all(directory);

  const auto rgba = RenderOffscreen(context, Extent, 3, directory);
  EXPECT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);
  for (std::uint32_t frame = 0; frame < 3; ++frame) {
    const auto path = SequenceFramePath(directory, frame);
    ASSERT_TRUE(std::filesystem::exists(path)) << path.string();
    EXPECT_GT(std::filesystem::file_size(path), 0U);
  }
  EXPECT_EQ(context.validationErrorCount(), 0U);
  std::filesystem::remove_all(directory);
}

TEST(HeadlessRenderTest, OrbitCameraChangesTheView) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{960, 540};
  const sim::Scene scene = sim::BuildScene(sim::ScenePreset::DamBreak);
  render::Camera camera;
  camera.setAspect(Extent);
  camera.frame(scene.params.domainMin, scene.params.domainMax);
  const auto front = RenderOffscreen(
      context,
      {.extent = Extent, .frames = 80, .scene = &scene, .camera = &camera, .simulate = true});
  camera.orbit(-1.15F, 0.22F);
  camera.dolly(-1.4F);
  const auto orbit = RenderOffscreen(
      context,
      {.extent = Extent, .frames = 80, .scene = &scene, .camera = &camera, .simulate = true});
  ASSERT_EQ(front.size(), orbit.size());
  int changed = 0;
  for (std::size_t i = 0; i < front.size(); ++i) {
    changed += front[i] != orbit[i] ? 1 : 0;
  }
  EXPECT_GT(changed, static_cast<int>(front.size() / 10));

  const auto dir = std::filesystem::temp_directory_path() / "sph-m7-camera";
  std::filesystem::create_directories(dir);
  WritePng(dir / "m7-camera-front.png", Extent, front);
  WritePng(dir / "m7-camera-orbit.png", Extent, orbit);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
