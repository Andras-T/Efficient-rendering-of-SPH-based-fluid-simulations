#include "render/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace sph::render {

namespace {

constexpr float MinDistance = 0.15F;
constexpr float MaxDistance = 80.0F;
constexpr float PitchLimit = 1.45F;
constexpr float MinFovY = 0.2F;
constexpr float MaxFovY = 2.2F;
constexpr float MinNear = 0.001F;
constexpr glm::vec3 WorldUp{0.0F, 1.0F, 0.0F};

float ClampPitch(float pitch) { return std::clamp(pitch, -PitchLimit, PitchLimit); }

}  // namespace

void Camera::setDistance(float distance) {
  distance_ = std::clamp(distance, MinDistance, MaxDistance);
}

void Camera::setYawPitch(float yaw, float pitch) {
  yaw_ = yaw;
  pitch_ = ClampPitch(pitch);
}

void Camera::setFovY(float fovY) { fovY_ = std::clamp(fovY, MinFovY, MaxFovY); }

void Camera::setNearPlane(float nearPlane) { nearPlane_ = std::max(nearPlane, MinNear); }

void Camera::setAspect(float aspect) { aspect_ = std::max(aspect, 0.05F); }

void Camera::setAspect(VkExtent2D extent) {
  if (extent.width == 0 || extent.height == 0) {
    return;
  }
  setAspect(static_cast<float>(extent.width) / static_cast<float>(extent.height));
}

void Camera::reset() { *this = Camera{}; }

void Camera::orbit(float deltaYaw, float deltaPitch) {
  setYawPitch(yaw_ + deltaYaw, pitch_ + deltaPitch);
}

void Camera::dolly(float delta) {
  const float scale = 1.0F - (delta * 0.15F);
  setDistance(distance_ * std::clamp(scale, 0.2F, 5.0F));
}

void Camera::pan(glm::vec2 localDelta) {
  const glm::vec3 direction = forward();
  glm::vec3 right = glm::cross(direction, WorldUp);
  if (glm::dot(right, right) < 1.0e-8F) {
    right = glm::cross(direction, glm::vec3{0.0F, 0.0F, 1.0F});
  }
  right = glm::normalize(right);
  const glm::vec3 up = glm::normalize(glm::cross(right, direction));
  const float speed = distance_ * 0.0025F;
  target_ += ((right * localDelta.x) + (up * localDelta.y)) * speed;
}

glm::vec3 Camera::position() const {
  const float cosPitch = std::cos(pitch_);
  const glm::vec3 offset{
      distance_ * cosPitch * std::sin(yaw_),
      distance_ * std::sin(pitch_),
      distance_ * cosPitch * std::cos(yaw_),
  };
  return target_ + offset;
}

glm::vec3 Camera::forward() const { return glm::normalize(target_ - position()); }

glm::mat4 Camera::view() const { return glm::lookAt(position(), target_, WorldUp); }

glm::mat4 Camera::projection() const {
  const float focal = 1.0F / std::tan(fovY_ * 0.5F);
  glm::mat4 matrix(0.0F);
  matrix[0][0] = focal / aspect_;
  matrix[1][1] = -focal;
  matrix[2][3] = -1.0F;
  matrix[3][2] = nearPlane_;
  return matrix;
}

}  // namespace sph::render
