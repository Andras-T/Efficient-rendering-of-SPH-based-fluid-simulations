#include "render/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>
#include <gtest/gtest.h>

#include <cmath>

namespace sph::render {
namespace {

TEST(CameraTest, OrbitKeepsDistanceAndLooksAtTarget) {
  Camera camera;
  camera.setTarget({1.0F, 2.0F, 3.0F});
  camera.setDistance(4.0F);
  camera.setYawPitch(0.3F, -0.2F);
  const float distance = camera.distance();
  camera.orbit(0.8F, -0.4F);

  EXPECT_NEAR(camera.distance(), distance, 1.0e-5F);
  EXPECT_NEAR(glm::length(camera.position() - camera.target()), distance, 1.0e-4F);
  const glm::vec3 toTarget = camera.target() - camera.position();
  EXPECT_GT(glm::dot(glm::normalize(toTarget), camera.forward()), 0.999F);
}

TEST(CameraTest, TruckMovesTargetAlongView) {
  Camera camera;
  camera.setYawPitch(0.0F, 0.0F);
  camera.setTarget({0.0F, 0.0F, 0.0F});
  camera.setDistance(3.0F);
  const glm::vec3 before = camera.position();
  camera.truck(0.0F, 0.0F, 1.0F);
  EXPECT_LT(camera.target().z, -0.5F);
  EXPECT_NEAR(camera.distance(), 3.0F, 1.0e-4F);
  EXPECT_GT(glm::length(camera.position() - before), 0.5F);
}

TEST(CameraTest, DollyAndPanMoveAsExpected) {
  Camera camera;
  camera.setYawPitch(0.0F, 0.0F);
  camera.setDistance(2.0F);
  camera.setTarget({0.0F, 0.0F, 0.0F});
  camera.dolly(2.0F);
  EXPECT_LT(camera.distance(), 2.0F);

  const glm::vec3 before = camera.target();
  camera.pan({20.0F, 0.0F});
  EXPECT_GT(std::abs(camera.target().x - before.x), 0.01F);
}

TEST(CameraTest, ReverseZNearPlaneMapsToOne) {
  Camera camera;
  camera.setTarget({0.0F, 0.0F, 0.0F});
  camera.setYawPitch(0.0F, 0.0F);
  camera.setDistance(5.0F);
  camera.setNearPlane(0.25F);
  camera.setAspect(1.0F);

  const glm::vec3 nearPoint = camera.position() + (camera.forward() * camera.nearPlane());
  const glm::vec4 clip = camera.viewProjection() * glm::vec4(nearPoint, 1.0F);
  EXPECT_NEAR(clip.z / clip.w, 1.0F, 1.0e-4F);

  const glm::vec3 farther = camera.position() + (camera.forward() * (camera.nearPlane() * 8.0F));
  const glm::vec4 farClip = camera.viewProjection() * glm::vec4(farther, 1.0F);
  EXPECT_LT(farClip.z / farClip.w, 0.2F);
  EXPECT_GT(farClip.z / farClip.w, 0.0F);
}

TEST(CameraTest, CenterRayHitsTargetPlane) {
  Camera camera;
  camera.setTarget({0.4F, 0.3F, -0.2F});
  camera.setYawPitch(0.55F, -0.25F);
  camera.setDistance(3.5F);
  camera.setAspect(16.0F / 9.0F);

  EXPECT_GT(glm::dot(camera.worldDirection({0.0F, 0.0F}), camera.forward()), 0.999F);

  const auto hit = camera.hitPlane({0.0F, 0.0F}, camera.target(), camera.forward());
  if (!hit.has_value()) {
    FAIL() << "center ray missed the target plane";
    return;
  }
  EXPECT_NEAR(glm::length(*hit - camera.target()), 0.0F, 1.0e-4F);
}

TEST(CameraTest, CenterRayHitsVolume) {
  Camera camera;
  camera.setTarget({0.0F, 0.4F, 0.0F});
  camera.setYawPitch(0.4F, -0.3F);
  camera.setDistance(3.0F);
  camera.setAspect(16.0F / 9.0F);
  const auto hit = camera.hitVolume({0.0F, 0.0F}, {-0.7F, 0.0F, -0.4F}, {0.7F, 1.0F, 0.4F});
  ASSERT_TRUE(hit.has_value());
  EXPECT_GE(hit->x, -0.7F);
  EXPECT_LE(hit->x, 0.7F);
  EXPECT_GE(hit->y, 0.0F);
  EXPECT_LE(hit->y, 1.0F);
}

TEST(CameraTest, AspectFollowsExtent) {
  Camera camera;
  camera.setAspect(VkExtent2D{1920, 1080});
  EXPECT_NEAR(camera.aspect(), 1920.0F / 1080.0F, 1.0e-6F);
  const float wide = camera.projection()[0][0];
  camera.setAspect(VkExtent2D{400, 400});
  EXPECT_NEAR(camera.aspect(), 1.0F, 1.0e-6F);
  EXPECT_GT(camera.projection()[0][0], wide);
}

}  // namespace
}  // namespace sph::render
