#include "render/Camera.hpp"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

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

struct Basis {
  glm::vec3 forward;
  glm::vec3 right;
  glm::vec3 up;
};

Basis CameraBasis(glm::vec3 forward) {
  glm::vec3 right = glm::cross(forward, WorldUp);
  if (glm::dot(right, right) < 1.0e-8F) {
    right = glm::cross(forward, glm::vec3{0.0F, 0.0F, 1.0F});
  }
  right = glm::normalize(right);
  const glm::vec3 up = glm::normalize(glm::cross(right, forward));
  return {forward, right, up};
}

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

void Camera::frame(glm::vec3 domainMin, glm::vec3 domainMax) {
  setTarget(0.5F * (domainMin + domainMax));
  setDistance(std::max(glm::length(domainMax - domainMin) * 1.15F, 1.0F));
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
  const Basis basis = CameraBasis(forward());
  const float speed = distance_ * 0.0025F;
  target_ += ((basis.right * localDelta.x) + (basis.up * localDelta.y)) * speed;
}

void Camera::truck(float right, float up, float forward) {
  const Basis basis = CameraBasis(this->forward());
  target_ += (basis.right * right) + (basis.up * up) + (basis.forward * forward);
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

glm::vec3 Camera::worldDirection(glm::vec2 ndc) const {
  const Basis basis = CameraBasis(forward());
  const float focal = 1.0F / std::tan(fovY_ * 0.5F);
  const glm::vec3 viewDir{ndc.x * aspect_ / focal, -ndc.y / focal, -1.0F};
  return glm::normalize((basis.right * viewDir.x) + (basis.up * viewDir.y) +
                        (basis.forward * (-viewDir.z)));
}

std::optional<glm::vec3> Camera::hitPlane(glm::vec2 ndc, glm::vec3 planePoint,
                                          glm::vec3 planeNormal) const {
  const glm::vec3 origin = position();
  const glm::vec3 direction = worldDirection(ndc);
  const float denom = glm::dot(direction, planeNormal);
  if (std::abs(denom) < 1.0e-6F) {
    return std::nullopt;
  }
  const float t = glm::dot(planePoint - origin, planeNormal) / denom;
  if (t < nearPlane_) {
    return std::nullopt;
  }
  return origin + (direction * t);
}

std::optional<glm::vec3> Camera::hitVolume(glm::vec2 ndc, glm::vec3 min, glm::vec3 max) const {
  const glm::vec3 origin = position();
  const glm::vec3 direction = worldDirection(ndc);
  float tEnter = nearPlane_;
  float tExit = 1.0e6F;
  for (int axis = 0; axis < 3; ++axis) {
    if (std::abs(direction[axis]) < 1.0e-8F) {
      if (origin[axis] < min[axis] || origin[axis] > max[axis]) {
        return std::nullopt;
      }
      continue;
    }
    float t0 = (min[axis] - origin[axis]) / direction[axis];
    float t1 = (max[axis] - origin[axis]) / direction[axis];
    if (t0 > t1) {
      std::swap(t0, t1);
    }
    tEnter = std::max(tEnter, t0);
    tExit = std::min(tExit, t1);
  }
  if (tExit < tEnter) {
    return std::nullopt;
  }
  return origin + direction * (tEnter + (tExit - tEnter) * 0.32F);
}

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
