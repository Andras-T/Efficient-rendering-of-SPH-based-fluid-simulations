#include "CubeMesh.hpp"
#include "app/Headless.hpp"
#include "app/ImageWriter.hpp"
#include "core/Capture.hpp"
#include "gpu/Context.hpp"
#include "render/Camera.hpp"
#include "render/DebugView.hpp"
#include "sim/Scene.hpp"
#include "ui/FluidGuide.hpp"
#include "ui/SmokeGuide.hpp"
#include "ui/WindGuide.hpp"

#include <gtest/gtest.h>

#include <cmath>
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

TEST(HeadlessRenderTest, GlassTankDiffersFromHiddenWalls) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{96, 54};
  const sim::Scene scene = sim::BuildScene(sim::ScenePreset::DamBreak);
  render::Camera camera;
  camera.setAspect(Extent);
  camera.frame(scene.params.domainMin, scene.params.domainMax);

  const auto hidden = RenderOffscreen(context, {.extent = Extent,
                                                .scene = &scene,
                                                .camera = &camera,
                                                .view = render::DebugView::Final,
                                                .walls = render::WallDisplay::Hidden});
  const auto glass = RenderOffscreen(context, {.extent = Extent,
                                               .scene = &scene,
                                               .camera = &camera,
                                               .view = render::DebugView::Final,
                                               .walls = render::WallDisplay::Glass});
  ASSERT_EQ(hidden.size(), glass.size());
  int changed = 0;
  for (std::size_t i = 0; i < hidden.size(); ++i) {
    changed += hidden[i] != glass[i] ? 1 : 0;
  }
  EXPECT_GT(changed, static_cast<int>(hidden.size() / 20));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, CylinderGlassDiffersFromCube) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{96, 54};
  const sim::Scene cube = sim::BuildScene(sim::ScenePreset::DamBreak);
  sim::SimParams cylinderParams;
  cylinderParams.tankShape = sim::TankShape::Cylinder;
  const sim::Scene cylinder = sim::BuildScene(sim::ScenePreset::DamBreak, cylinderParams);
  render::Camera camera;
  camera.setAspect(Extent);
  camera.frame(cube.params.domainMin, cube.params.domainMax);

  const auto cubePixels = RenderOffscreen(context, {.extent = Extent,
                                                    .scene = &cube,
                                                    .camera = &camera,
                                                    .view = render::DebugView::Final,
                                                    .walls = render::WallDisplay::Glass});
  const auto cylinderPixels = RenderOffscreen(context, {.extent = Extent,
                                                        .scene = &cylinder,
                                                        .camera = &camera,
                                                        .view = render::DebugView::Final,
                                                        .walls = render::WallDisplay::Glass});
  ASSERT_EQ(cubePixels.size(), cylinderPixels.size());
  int changed = 0;
  for (std::size_t i = 0; i < cubePixels.size(); ++i) {
    changed += cubePixels[i] != cylinderPixels[i] ? 1 : 0;
  }
  EXPECT_GT(changed, static_cast<int>(cubePixels.size() / 20));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, PaddleMarksTheCursor) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{64, 64};
  sim::Scene scene;
  scene.params.domainMin = {-0.4F, 0.0F, -0.4F};
  scene.params.domainMax = {0.4F, 0.8F, 0.4F};
  render::Camera camera;
  camera.setAspect(Extent);
  camera.setTarget({0.0F, 0.3F, 0.0F});
  camera.setYawPitch(0.0F, 0.0F);
  camera.setDistance(1.2F);
  const auto rgba = RenderOffscreen(context, {.extent = Extent,
                                              .scene = &scene,
                                              .camera = &camera,
                                              .walls = render::WallDisplay::Hidden,
                                              .paddleCenter = {0.0F, 0.3F, 0.0F},
                                              .paddleRadius = 0.18F});
  ASSERT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);
  const Pixel center = PixelAt(rgba, Extent, Extent.width / 2, Extent.height / 2);
  EXPECT_GT(center.g, center.r + 20);
  EXPECT_GT(center.b, center.r + 20);
  EXPECT_EQ(context.validationErrorCount(), 0U);
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

TEST(HeadlessRenderTest, FluidGuideStepsIsolateTheStage) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{96, 54};
  const sim::Scene scene = sim::BuildScene(sim::ScenePreset::DamBreak);
  render::Camera camera;
  camera.setAspect(Extent);
  camera.frame(scene.params.domainMin, scene.params.domainMax);
  const auto particles = RenderOffscreen(context, {.extent = Extent,
                                                   .frames = 24,
                                                   .scene = &scene,
                                                   .camera = &camera,
                                                   .simulate = true,
                                                   .view = ui::FluidGuideAt(0).view,
                                                   .walls = render::WallDisplay::Glass});
  const auto density = RenderOffscreen(context, {.extent = Extent,
                                                 .frames = 24,
                                                 .scene = &scene,
                                                 .camera = &camera,
                                                 .simulate = true,
                                                 .view = ui::FluidGuideAt(2).view,
                                                 .walls = render::WallDisplay::Glass});
  ASSERT_EQ(particles.size(), density.size());
  int changed = 0;
  for (std::size_t i = 0; i < particles.size(); ++i) {
    changed += particles[i] != density[i] ? 1 : 0;
  }
  EXPECT_EQ(ui::FluidGuideAt(0).view, render::DebugView::Particles);
  EXPECT_EQ(ui::FluidGuideAt(2).view, render::DebugView::Density);
  EXPECT_GT(changed, static_cast<int>(particles.size() / 20));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, WindTunnelCarIsVisible) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{96, 54};
  const auto rgba = RenderOffscreen(context, {.extent = Extent,
                                              .frames = 24,
                                              .simulate = true,
                                              .view = render::DebugView::Final,
                                              .simulation = SimulationKind::WindTunnel});
  ASSERT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);
  int carPixels = 0;
  for (std::size_t i = 0; i + 3 < rgba.size(); i += 4) {
    const int r = rgba[i];
    const int g = rgba[i + 1];
    const int b = rgba[i + 2];
    if ((r > 50 && r > g + 12 && r > b + 8) || (b > 50 && b > r + 12 && b > g + 4)) {
      ++carPixels;
    }
  }
  EXPECT_GT(carPixels, 40);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, SmokePlumeIsVisible) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{96, 54};
  const auto rgba = RenderOffscreen(context, {.extent = Extent,
                                              .frames = 36,
                                              .simulate = true,
                                              .view = render::DebugView::Final,
                                              .simulation = SimulationKind::Smoke});
  ASSERT_EQ(rgba.size(), std::size_t{Extent.width} * Extent.height * 4);
  int smokePixels = 0;
  for (std::size_t i = 0; i + 3 < rgba.size(); i += 4) {
    const int r = rgba[i];
    const int g = rgba[i + 1];
    const int b = rgba[i + 2];
    if (r > 70 && g > 65 && b > 50 && std::abs(r - g) < 45) {
      ++smokePixels;
    }
  }
  EXPECT_GT(smokePixels, 40);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, SmokeVelocityViewDiffersFromFinal) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{80, 48};
  const auto finalView = RenderOffscreen(context, {.extent = Extent,
                                                   .frames = 28,
                                                   .simulate = true,
                                                   .view = render::DebugView::Final,
                                                   .simulation = SimulationKind::Smoke});
  const auto velocity = RenderOffscreen(context, {.extent = Extent,
                                                  .frames = 28,
                                                  .simulate = true,
                                                  .view = render::DebugView::Velocity,
                                                  .simulation = SimulationKind::Smoke});
  ASSERT_EQ(finalView.size(), velocity.size());
  int changed = 0;
  for (std::size_t i = 0; i < finalView.size(); ++i) {
    changed += finalView[i] != velocity[i] ? 1 : 0;
  }
  EXPECT_GT(changed, static_cast<int>(finalView.size() / 40));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, WindVelocityViewDiffersFromFinal) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{80, 48};
  const auto finalView = RenderOffscreen(context, {.extent = Extent,
                                                   .frames = 20,
                                                   .simulate = true,
                                                   .view = render::DebugView::Final,
                                                   .simulation = SimulationKind::WindTunnel});
  const auto velocity = RenderOffscreen(context, {.extent = Extent,
                                                  .frames = 20,
                                                  .simulate = true,
                                                  .view = render::DebugView::Velocity,
                                                  .simulation = SimulationKind::WindTunnel});
  ASSERT_EQ(finalView.size(), velocity.size());
  int changed = 0;
  for (std::size_t i = 0; i < finalView.size(); ++i) {
    changed += finalView[i] != velocity[i] ? 1 : 0;
  }
  EXPECT_GT(changed, static_cast<int>(finalView.size() / 40));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, WindGuideStepsIsolateTheStage) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{80, 48};
  const ui::WindGuideStep& tunnel = ui::WindGuideAt(0);
  const ui::WindGuideStep& car = ui::WindGuideAt(1);
  const auto empty = RenderOffscreen(context, {.extent = Extent,
                                               .frames = 16,
                                               .simulate = true,
                                               .view = tunnel.view,
                                               .simulation = SimulationKind::WindTunnel,
                                               .windCar = tunnel.showCar,
                                               .windTracers = tunnel.showTracers,
                                               .windVolume = tunnel.showVolume});
  const auto body = RenderOffscreen(context, {.extent = Extent,
                                              .frames = 16,
                                              .simulate = true,
                                              .view = car.view,
                                              .simulation = SimulationKind::WindTunnel,
                                              .windCar = car.showCar,
                                              .windTracers = car.showTracers,
                                              .windVolume = car.showVolume});
  ASSERT_EQ(empty.size(), body.size());
  int changed = 0;
  for (std::size_t i = 0; i < empty.size(); ++i) {
    changed += empty[i] != body[i] ? 1 : 0;
  }
  EXPECT_FALSE(tunnel.showCar);
  EXPECT_TRUE(car.showCar);
  EXPECT_GT(changed, static_cast<int>(empty.size() / 40));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, SmokeGuideStepsIsolateTheStage) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{80, 48};
  const ui::SmokeGuideStep& domain = ui::SmokeGuideAt(0);
  const ui::SmokeGuideStep& finalStep = ui::SmokeGuideAt(ui::SmokeGuideStepCount() - 1U);
  const auto empty = RenderOffscreen(context, {.extent = Extent,
                                               .frames = 20,
                                               .simulate = true,
                                               .view = domain.view,
                                               .simulation = SimulationKind::Smoke,
                                               .smokeVolume = domain.showVolume,
                                               .smokeClipY = domain.clipY});
  const auto plume = RenderOffscreen(context, {.extent = Extent,
                                               .frames = 20,
                                               .simulate = true,
                                               .view = finalStep.view,
                                               .simulation = SimulationKind::Smoke,
                                               .smokeVolume = finalStep.showVolume,
                                               .smokeClipY = finalStep.clipY});
  ASSERT_EQ(empty.size(), plume.size());
  int changed = 0;
  for (std::size_t i = 0; i < empty.size(); ++i) {
    changed += empty[i] != plume[i] ? 1 : 0;
  }
  EXPECT_FALSE(domain.showVolume);
  EXPECT_TRUE(finalStep.showVolume);
  EXPECT_GT(changed, static_cast<int>(empty.size() / 40));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, WindVolumeViewDiffersFromTracers) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{80, 48};
  const auto volume = RenderOffscreen(context, {.extent = Extent,
                                                .frames = 28,
                                                .simulate = true,
                                                .view = render::DebugView::Final,
                                                .simulation = SimulationKind::WindTunnel,
                                                .windCar = true,
                                                .windTracers = false,
                                                .windVolume = true});
  const auto tracers = RenderOffscreen(context, {.extent = Extent,
                                                 .frames = 28,
                                                 .simulate = true,
                                                 .view = render::DebugView::Final,
                                                 .simulation = SimulationKind::WindTunnel,
                                                 .windCar = true,
                                                 .windTracers = true,
                                                 .windVolume = false});
  ASSERT_EQ(volume.size(), tracers.size());
  int changed = 0;
  int fieldPixels = 0;
  for (std::size_t i = 0; i + 3 < volume.size(); i += 4) {
    changed += (volume[i] != tracers[i] || volume[i + 1] != tracers[i + 1] ||
                volume[i + 2] != tracers[i + 2])
                   ? 1
                   : 0;
    const int r = volume[i];
    const int g = volume[i + 1];
    const int b = volume[i + 2];
    if (b > r + 8 && b > 70 && g > 50) {
      ++fieldPixels;
    }
  }
  EXPECT_GT(changed, static_cast<int>(volume.size() / 40));
  EXPECT_GT(fieldPixels, 20);
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

TEST(HeadlessRenderTest, WindImportedMeshReplacesTheCar) {
  const gpu::Context context({.validation = true});
  constexpr VkExtent2D Extent{80, 48};
  const auto directory = std::filesystem::temp_directory_path() / "sph-wind-mesh";
  std::filesystem::create_directories(directory);
  const auto cube = test::WriteCubeObj(directory);
  const auto car = RenderOffscreen(context, {.extent = Extent,
                                             .frames = 20,
                                             .simulate = true,
                                             .view = render::DebugView::Final,
                                             .simulation = SimulationKind::WindTunnel,
                                             .windCar = true,
                                             .windTracers = false,
                                             .windVolume = false});
  const auto mesh = RenderOffscreen(context, {.extent = Extent,
                                              .frames = 20,
                                              .simulate = true,
                                              .view = render::DebugView::Final,
                                              .simulation = SimulationKind::WindTunnel,
                                              .windCar = true,
                                              .windTracers = false,
                                              .windVolume = false,
                                              .meshPath = cube});
  ASSERT_EQ(car.size(), mesh.size());
  int changed = 0;
  for (std::size_t i = 0; i < car.size(); ++i) {
    changed += car[i] != mesh[i] ? 1 : 0;
  }
  EXPECT_GT(changed, static_cast<int>(car.size() / 40));
  EXPECT_EQ(context.validationErrorCount(), 0U);
}

}  // namespace
}  // namespace sph
