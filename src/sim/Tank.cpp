#include "sim/Tank.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>

namespace sph::sim {
namespace {

constexpr float Sqrt3Over2 = 0.8660254037844386F;

glm::vec2 HexagonNormal(int axis, float sign) {
  if (axis == 0) {
    return {sign, 0.0F};
  }
  if (axis == 1) {
    return {sign * 0.5F, sign * Sqrt3Over2};
  }
  return {sign * -0.5F, sign * Sqrt3Over2};
}

}  // namespace

const char* TankShapeName(TankShape shape) {
  switch (shape) {
    case TankShape::Cylinder:
      return "cylinder";
    case TankShape::Hexagon:
      return "hexagon";
    default:
      return "cube";
  }
}

TankShape ParseTankShape(std::string_view name) {
  if (name == "cube" || name == "box") {
    return TankShape::Box;
  }
  if (name == "cylinder" || name == "cyl") {
    return TankShape::Cylinder;
  }
  if (name == "hexagon" || name == "hex") {
    return TankShape::Hexagon;
  }
  throw std::invalid_argument(std::format("unknown tank '{}'", name));
}

glm::vec2 TankCenterXz(const SimParams& params) {
  return 0.5F * (glm::vec2(params.domainMin.x, params.domainMin.z) +
                 glm::vec2(params.domainMax.x, params.domainMax.z));
}

float TankRadius(const SimParams& params) {
  const glm::vec3 extent = params.domainMax - params.domainMin;
  return 0.5F * std::min(extent.x, extent.z);
}

float TankApothem(const SimParams& params) { return TankRadius(params) * Sqrt3Over2; }

float HexagonNorm(glm::vec2 delta) {
  const float a = std::abs(delta.x);
  const float b = std::abs(0.5F * delta.x + Sqrt3Over2 * delta.y);
  const float c = std::abs(-0.5F * delta.x + Sqrt3Over2 * delta.y);
  return std::max(a, std::max(b, c));
}

bool InsideTank(glm::vec3 position, const SimParams& params, float inset) {
  if (position.y < params.domainMin.y + inset || position.y > params.domainMax.y - inset) {
    return false;
  }
  if (params.tankShape == TankShape::Box) {
    return position.x >= params.domainMin.x + inset && position.x <= params.domainMax.x - inset &&
           position.z >= params.domainMin.z + inset && position.z <= params.domainMax.z - inset;
  }
  const glm::vec2 delta = glm::vec2(position.x, position.z) - TankCenterXz(params);
  if (params.tankShape == TankShape::Hexagon) {
    return HexagonNorm(delta) <= TankApothem(params) - inset;
  }
  return glm::length(delta) <= TankRadius(params) - inset;
}

void CollideTank(glm::vec3& position, glm::vec3& velocity, const SimParams& params, float radius) {
  const glm::vec3 minBound = params.domainMin + glm::vec3(radius);
  const glm::vec3 maxBound = params.domainMax - glm::vec3(radius);
  if (!std::isfinite(position.x) || !std::isfinite(velocity.x)) {
    position = 0.5F * (minBound + maxBound);
    velocity = glm::vec3{0.0F};
    return;
  }

  const int axes = params.tankShape == TankShape::Box ? 3 : 1;
  for (int axis = 0; axis < 3; ++axis) {
    if (axes == 1 && axis != 1) {
      continue;
    }
    if (position[axis] < minBound[axis]) {
      position[axis] = minBound[axis];
      if (velocity[axis] < 0.0F) {
        velocity[axis] *= -params.restitution;
      }
    } else if (position[axis] > maxBound[axis]) {
      position[axis] = maxBound[axis];
      if (velocity[axis] > 0.0F) {
        velocity[axis] *= -params.restitution;
      }
    }
  }

  if (params.tankShape == TankShape::Box) {
    return;
  }

  const glm::vec2 center = TankCenterXz(params);
  glm::vec2 delta = glm::vec2(position.x, position.z) - center;
  if (params.tankShape == TankShape::Cylinder) {
    const float inner = std::max(TankRadius(params) - radius, radius);
    const float dist = glm::length(delta);
    if (dist > inner) {
      const glm::vec2 normal = dist > 1.0e-6F ? delta / dist : glm::vec2{1.0F, 0.0F};
      const glm::vec2 xz = center + normal * inner;
      position.x = xz.x;
      position.z = xz.y;
      const float approach = glm::dot(glm::vec2(velocity.x, velocity.z), normal);
      if (approach > 0.0F) {
        velocity.x -= normal.x * approach * (1.0F + params.restitution);
        velocity.z -= normal.y * approach * (1.0F + params.restitution);
      }
    }
    return;
  }

  const float inner = std::max(TankApothem(params) - radius, radius);
  for (int pass = 0; pass < 3; ++pass) {
    delta = glm::vec2(position.x, position.z) - center;
    const float n0 = glm::dot(delta, HexagonNormal(0, 1.0F));
    const float n1 = glm::dot(delta, HexagonNormal(1, 1.0F));
    const float n2 = glm::dot(delta, HexagonNormal(2, 1.0F));
    const float a0 = std::abs(n0);
    const float a1 = std::abs(n1);
    const float a2 = std::abs(n2);
    const float mag = std::max(a0, std::max(a1, a2));
    if (mag <= inner) {
      return;
    }
    int axis = 0;
    float signedN = n0;
    if (a1 >= a0 && a1 >= a2) {
      axis = 1;
      signedN = n1;
    } else if (a2 >= a0 && a2 >= a1) {
      axis = 2;
      signedN = n2;
    }
    const glm::vec2 normal = HexagonNormal(axis, signedN >= 0.0F ? 1.0F : -1.0F);
    const glm::vec2 xz = glm::vec2(position.x, position.z) - normal * (mag - inner);
    position.x = xz.x;
    position.z = xz.y;
    const float approach = glm::dot(glm::vec2(velocity.x, velocity.z), normal);
    if (approach > 0.0F) {
      velocity.x -= normal.x * approach * (1.0F + params.restitution);
      velocity.z -= normal.y * approach * (1.0F + params.restitution);
    }
  }
}

void ClampAabbToTank(glm::vec3& center, glm::vec3 half, const SimParams& params) {
  const glm::vec3 lo = params.domainMin + half;
  const glm::vec3 hi = params.domainMax - half;
  center = glm::clamp(center, lo, hi);
  if (params.tankShape == TankShape::Box) {
    return;
  }
  const glm::vec2 mid = TankCenterXz(params);
  glm::vec2 delta = glm::vec2(center.x, center.z) - mid;
  if (params.tankShape == TankShape::Cylinder) {
    const float inner = std::max(TankRadius(params) - glm::length(glm::vec2(half.x, half.z)), 0.0F);
    const float dist = glm::length(delta);
    if (dist > inner && dist > 1.0e-6F) {
      delta *= inner / dist;
      center.x = mid.x + delta.x;
      center.z = mid.y + delta.y;
    }
    return;
  }
  float corner = 0.0F;
  for (const float sx : {-1.0F, 1.0F}) {
    for (const float sz : {-1.0F, 1.0F}) {
      corner = std::max(corner, HexagonNorm({sx * half.x, sz * half.z}));
    }
  }
  const float inner = std::max(TankApothem(params) - corner, 0.0F);
  const float mag = HexagonNorm(delta);
  if (mag > inner && mag > 1.0e-6F) {
    delta *= inner / mag;
    center.x = mid.x + delta.x;
    center.z = mid.y + delta.y;
  }
}

}  // namespace sph::sim
