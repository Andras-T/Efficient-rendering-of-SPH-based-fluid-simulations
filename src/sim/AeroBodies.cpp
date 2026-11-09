#include "sim/AeroBodies.hpp"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace sph::sim {
namespace {

constexpr glm::vec3 BodyColor{0.82F, 0.84F, 0.88F};

void PushTriangle(std::vector<MeshVertex>& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
  const glm::vec3 ab = b - a;
  const glm::vec3 ac = c - a;
  const glm::vec3 crossed = glm::cross(ab, ac);
  const float length = glm::length(crossed);
  if (length < 1.0e-12F) {
    return;
  }
  const glm::vec3 normal = crossed / length;
  mesh.push_back({.position = a, .normal = normal, .color = BodyColor});
  mesh.push_back({.position = b, .normal = normal, .color = BodyColor});
  mesh.push_back({.position = c, .normal = normal, .color = BodyColor});
}

void PushQuad(std::vector<MeshVertex>& mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
  PushTriangle(mesh, a, b, c);
  PushTriangle(mesh, a, c, d);
}

glm::vec3 SpherePoint(float radius, float theta, float phi) {
  const float sinPhi = std::sin(phi);
  return {radius * sinPhi * std::cos(theta), radius * (1.0F - std::cos(phi)),
          radius * sinPhi * std::sin(theta)};
}

}  // namespace

float SphereCdSchillerNaumann(float reynolds) {
  const float re = std::max(reynolds, 1.0e-6F);
  return 24.0F / re * (1.0F + 0.15F * std::pow(re, 0.687F));
}

std::vector<MeshVertex> BuildBenchmarkCube() {
  std::vector<MeshVertex> mesh;
  mesh.reserve(36);
  const glm::vec3 a{-0.5F, 0.0F, -0.5F};
  const glm::vec3 b{0.5F, 0.0F, -0.5F};
  const glm::vec3 c{0.5F, 1.0F, -0.5F};
  const glm::vec3 d{-0.5F, 1.0F, -0.5F};
  const glm::vec3 e{-0.5F, 0.0F, 0.5F};
  const glm::vec3 f{0.5F, 0.0F, 0.5F};
  const glm::vec3 g{0.5F, 1.0F, 0.5F};
  const glm::vec3 h{-0.5F, 1.0F, 0.5F};
  PushQuad(mesh, a, d, c, b);  // -z
  PushQuad(mesh, e, f, g, h);  // +z
  PushQuad(mesh, a, b, f, e);  // -y
  PushQuad(mesh, d, h, g, c);  // +y
  PushQuad(mesh, a, e, h, d);  // -x
  PushQuad(mesh, b, c, g, f);  // +x
  return mesh;
}

std::vector<MeshVertex> BuildBenchmarkSphere() {
  constexpr std::uint32_t Slices = 16;
  constexpr std::uint32_t Stacks = 10;
  constexpr float Radius = 0.5F;
  constexpr float TwoPi = 2.0F * std::numbers::pi_v<float>;
  std::vector<MeshVertex> mesh;
  mesh.reserve(static_cast<std::size_t>(Slices) * Stacks * 6);
  for (std::uint32_t stack = 0; stack < Stacks; ++stack) {
    const float phi0 = std::numbers::pi_v<float> * static_cast<float>(stack) / Stacks;
    const float phi1 = std::numbers::pi_v<float> * static_cast<float>(stack + 1) / Stacks;
    for (std::uint32_t slice = 0; slice < Slices; ++slice) {
      const float theta0 = TwoPi * static_cast<float>(slice) / Slices;
      const float theta1 = TwoPi * static_cast<float>(slice + 1) / Slices;
      const glm::vec3 p00 = SpherePoint(Radius, theta0, phi0);
      const glm::vec3 p10 = SpherePoint(Radius, theta1, phi0);
      const glm::vec3 p01 = SpherePoint(Radius, theta0, phi1);
      const glm::vec3 p11 = SpherePoint(Radius, theta1, phi1);
      if (stack > 0) {
        PushTriangle(mesh, p00, p10, p11);
      }
      if (stack + 1 < Stacks) {
        PushTriangle(mesh, p00, p11, p01);
      }
    }
  }
  return mesh;
}

std::vector<MeshVertex> BuildBenchmarkAhmed() {
  // Ahmed 1984 1:1 metres, 25° rear slant, no stilts. FitMeshToTunnel places it.
  constexpr float Length = 1.044F;
  constexpr float Width = 0.389F;
  constexpr float Height = 0.288F;
  constexpr float SlantLength = 0.222F;
  const float slantDrop = SlantLength * std::tan(25.0F * std::numbers::pi_v<float> / 180.0F);
  const float rearHeight = Height - slantDrop;
  const float halfW = 0.5F * Width;
  const float slantX = Length - SlantLength;
  const glm::vec3 noseBottomLeft{0.0F, 0.0F, -halfW};
  const glm::vec3 noseBottomRight{0.0F, 0.0F, halfW};
  const glm::vec3 noseTopLeft{0.0F, Height, -halfW};
  const glm::vec3 noseTopRight{0.0F, Height, halfW};
  const glm::vec3 tailBottomLeft{Length, 0.0F, -halfW};
  const glm::vec3 tailBottomRight{Length, 0.0F, halfW};
  const glm::vec3 tailTopLeft{Length, rearHeight, -halfW};
  const glm::vec3 tailTopRight{Length, rearHeight, halfW};
  const glm::vec3 roofLeft{slantX, Height, -halfW};
  const glm::vec3 roofRight{slantX, Height, halfW};

  std::vector<MeshVertex> mesh;
  mesh.reserve(48);
  PushQuad(mesh, noseBottomLeft, noseBottomRight, noseTopRight, noseTopLeft);
  PushQuad(mesh, tailBottomLeft, tailTopLeft, tailTopRight, tailBottomRight);
  PushQuad(mesh, noseBottomLeft, tailBottomLeft, tailBottomRight, noseBottomRight);
  PushQuad(mesh, noseTopLeft, noseTopRight, roofRight, roofLeft);
  PushQuad(mesh, roofLeft, roofRight, tailTopRight, tailTopLeft);
  PushTriangle(mesh, noseBottomLeft, tailTopLeft, tailBottomLeft);
  PushTriangle(mesh, noseBottomLeft, roofLeft, tailTopLeft);
  PushTriangle(mesh, noseBottomLeft, noseTopLeft, roofLeft);
  PushTriangle(mesh, noseBottomRight, tailBottomRight, tailTopRight);
  PushTriangle(mesh, noseBottomRight, tailTopRight, roofRight);
  PushTriangle(mesh, noseBottomRight, roofRight, noseTopRight);
  return mesh;
}

void PlaceBluffBody(std::vector<MeshVertex>& mesh, const CarDomain& domain) {
  if (mesh.empty()) {
    return;
  }
  glm::vec3 mn = mesh.front().position;
  glm::vec3 mx = mn;
  for (const MeshVertex& vertex : mesh) {
    mn = glm::min(mn, vertex.position);
    mx = glm::max(mx, vertex.position);
  }
  const glm::vec3 extent = glm::max(mx - mn, glm::vec3(1.0e-4F));
  const glm::vec3 domainSize = domain.cellSize * glm::vec3(domain.grid);
  const float targetY = domain.cellSize * 9.0F;
  const float targetX = domain.cellSize * 20.0F;
  const float targetZ = domain.cellSize * 12.0F;
  const float scale = std::min({targetY / extent.y, targetX / extent.x, targetZ / extent.z, 8.0F});
  const glm::vec3 center = 0.5F * (mn + mx);
  const glm::vec3 tunnelMid = domain.origin + 0.5F * domainSize;
  const float floorY = domain.origin.y + domain.cellSize * 1.25F;
  const glm::vec3 scaledMin = (mn - center) * scale;
  const glm::vec3 translation{domain.origin.x + domain.cellSize * 36.0F, floorY - scaledMin.y,
                              tunnelMid.z};
  for (MeshVertex& vertex : mesh) {
    vertex.position = (vertex.position - center) * scale + translation;
  }
}

void WriteMeshObj(const std::filesystem::path& path, std::span<const MeshVertex> mesh) {
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("failed to write " + path.string());
  }
  out << "o body\n";
  for (const MeshVertex& vertex : mesh) {
    out << "v " << vertex.position.x << ' ' << vertex.position.y << ' ' << vertex.position.z
        << '\n';
  }
  for (std::size_t i = 0; i + 2 < mesh.size(); i += 3) {
    out << "f " << (i + 1) << ' ' << (i + 2) << ' ' << (i + 3) << '\n';
  }
}

BodyOccupancy MeasureSolidOccupancy(const CarDomain& domain,
                                    std::span<const std::uint32_t> occupancy) {
  BodyOccupancy measured;
  std::uint32_t minX = domain.grid.x;
  std::uint32_t minY = domain.grid.y;
  std::uint32_t minZ = domain.grid.z;
  std::uint32_t maxX = 0;
  std::uint32_t maxY = 0;
  std::uint32_t maxZ = 0;
  std::vector<std::uint8_t> seen(
      static_cast<std::size_t>(domain.grid.y) * static_cast<std::size_t>(domain.grid.z), 0);
  for (std::uint32_t z = 0; z < domain.grid.z; ++z) {
    for (std::uint32_t y = 0; y < domain.grid.y; ++y) {
      for (std::uint32_t x = 0; x < domain.grid.x; ++x) {
        const std::uint32_t index = x + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
        if (occupancy[index] != static_cast<std::uint32_t>(WindCell::Solid)) {
          continue;
        }
        ++measured.solids;
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        minZ = std::min(minZ, z);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        maxZ = std::max(maxZ, z);
        const std::uint32_t silhouette = y + z * domain.grid.y;
        if (seen.at(silhouette) == 0) {
          seen.at(silhouette) = 1;
          ++measured.frontalCells;
        }
      }
    }
  }
  if (measured.solids == 0) {
    return measured;
  }
  measured.extentX = maxX - minX + 1;
  measured.extentY = maxY - minY + 1;
  measured.extentZ = maxZ - minZ + 1;
  return measured;
}

}  // namespace sph::sim
