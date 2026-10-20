#pragma once

#include "gpu/Vulkan.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace sph::render {

// Orbit camera: yaw/pitch around a target, reverse-Z Vulkan projection.
class Camera {
 public:
  void setTarget(glm::vec3 target) { target_ = target; }
  void setDistance(float distance);
  void setYawPitch(float yaw, float pitch);
  void setFovY(float fovY);
  void setNearPlane(float nearPlane);
  void setAspect(float aspect);
  void setAspect(VkExtent2D extent);
  void frame(glm::vec3 domainMin, glm::vec3 domainMax);
  void reset();

  void orbit(float deltaYaw, float deltaPitch);
  void dolly(float delta);
  void pan(glm::vec2 localDelta);
  void truck(float right, float up, float forward);

  [[nodiscard]] glm::vec3 target() const { return target_; }
  [[nodiscard]] float distance() const { return distance_; }
  [[nodiscard]] float yaw() const { return yaw_; }
  [[nodiscard]] float pitch() const { return pitch_; }
  [[nodiscard]] float fovY() const { return fovY_; }
  [[nodiscard]] float aspect() const { return aspect_; }
  [[nodiscard]] float nearPlane() const { return nearPlane_; }
  [[nodiscard]] glm::vec3 position() const;
  [[nodiscard]] glm::vec3 forward() const;
  // `ndc` is Vulkan clip XY in [-1, 1] (y down).
  [[nodiscard]] glm::vec3 worldDirection(glm::vec2 ndc) const;
  [[nodiscard]] std::optional<glm::vec3> hitPlane(glm::vec2 ndc, glm::vec3 planePoint,
                                                  glm::vec3 planeNormal) const;
  // Point along the mouse ray inside `min`/`max`, biased toward the camera.
  [[nodiscard]] std::optional<glm::vec3> hitVolume(glm::vec2 ndc, glm::vec3 min,
                                                   glm::vec3 max) const;
  [[nodiscard]] glm::mat4 view() const;
  [[nodiscard]] glm::mat4 projection() const;
  [[nodiscard]] glm::mat4 viewProjection() const { return projection() * view(); }

 private:
  glm::vec3 target_{0.0F, 0.35F, 0.0F};
  float distance_ = 3.0F;
  float yaw_ = 0.45F;
  float pitch_ = -0.35F;
  float fovY_ = 0.87266463F;  // 50 degrees
  float nearPlane_ = 0.05F;
  float aspect_ = 16.0F / 9.0F;
};

}  // namespace sph::render
