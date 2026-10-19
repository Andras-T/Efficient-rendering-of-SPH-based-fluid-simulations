#include "app/Headless.hpp"
#include "gpu/Context.hpp"
#include "render/Camera.hpp"
#include "sim/Scene.hpp"

#include <gtest/gtest.h>

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

TEST(ParticleRenderTest, SphereImpostorShadesTheCenter) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{64, 64};

  sim::Scene scene;
  scene.params.particleSpacing = 0.08F;
  sim::Particle particle;
  particle.position = {0.0F, 0.0F, 0.0F};
  particle.mass = scene.params.mass();
  particle.density = scene.params.restDensity;
  scene.particles.push_back(particle);

  render::Camera camera;
  camera.setAspect(Extent);
  camera.setTarget({0.0F, 0.0F, 0.0F});
  camera.setYawPitch(0.0F, 0.0F);
  camera.setDistance(0.55F);

  const auto rgba =
      RenderOffscreen(context, {.extent = Extent, .scene = &scene, .camera = &camera});
  ASSERT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);

  const Pixel center = PixelAt(rgba, Extent, Extent.width / 2, Extent.height / 2);
  EXPECT_GT(center.b, center.r + 30);
  EXPECT_GT(center.b, 60);

  const Pixel corner = PixelAt(rgba, Extent, 1, 1);
  EXPECT_LT(corner.b, center.b);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
