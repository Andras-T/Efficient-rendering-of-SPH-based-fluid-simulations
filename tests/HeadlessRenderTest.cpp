#include "app/Headless.hpp"
#include "core/Capture.hpp"
#include "gpu/Context.hpp"

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

TEST(HeadlessRenderTest, BackgroundIsVerticalGradient) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{64, 48};
  const auto rgba = RenderOffscreen(context, Extent, 2);
  ASSERT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);

  const Pixel top = PixelAt(rgba, Extent, Extent.width / 2, 0);
  const Pixel bottom = PixelAt(rgba, Extent, Extent.width / 2, Extent.height - 1);
  EXPECT_LT(top.r, bottom.r);
  EXPECT_LT(top.g, bottom.g);
  EXPECT_LT(top.b, bottom.b);

  const Pixel left = PixelAt(rgba, Extent, 0, Extent.height / 2);
  const Pixel right = PixelAt(rgba, Extent, Extent.width - 1, Extent.height / 2);
  EXPECT_LE(std::abs(left.r - right.r), 1);
  EXPECT_LE(std::abs(left.b - right.b), 1);
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

}  // namespace
}  // namespace sph
