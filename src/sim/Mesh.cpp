#include "sim/Mesh.hpp"

#include "core/Log.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <queue>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>

#ifndef SPH_FORMULA_MESH_PATH
#define SPH_FORMULA_MESH_PATH ""
#endif

namespace sph::sim {
namespace {

std::string LowerExtension(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  for (char& c : ext) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return ext;
}

float DistPointTriangleSq(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
  const glm::vec3 ab = b - a;
  const glm::vec3 ac = c - a;
  const glm::vec3 ap = p - a;
  const float d1 = glm::dot(ab, ap);
  const float d2 = glm::dot(ac, ap);
  if (d1 <= 0.0F && d2 <= 0.0F) {
    return glm::dot(ap, ap);
  }
  const glm::vec3 bp = p - b;
  const float d3 = glm::dot(ab, bp);
  const float d4 = glm::dot(ac, bp);
  if (d3 >= 0.0F && d4 <= d3) {
    return glm::dot(bp, bp);
  }
  const float vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0F && d1 >= 0.0F && d3 <= 0.0F) {
    const float v = d1 / (d1 - d3);
    const glm::vec3 closest = a + v * ab;
    const glm::vec3 d = p - closest;
    return glm::dot(d, d);
  }
  const glm::vec3 cp = p - c;
  const float d5 = glm::dot(ab, cp);
  const float d6 = glm::dot(ac, cp);
  if (d6 >= 0.0F && d5 <= d6) {
    return glm::dot(cp, cp);
  }
  const float vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0F && d2 >= 0.0F && d6 <= 0.0F) {
    const float w = d2 / (d2 - d6);
    const glm::vec3 closest = a + w * ac;
    const glm::vec3 d = p - closest;
    return glm::dot(d, d);
  }
  const float va = d3 * d6 - d5 * d4;
  if (va <= 0.0F && (d4 - d3) >= 0.0F && (d5 - d6) >= 0.0F) {
    const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    const glm::vec3 closest = b + w * (c - b);
    const glm::vec3 d = p - closest;
    return glm::dot(d, d);
  }
  const float denom = 1.0F / (va + vb + vc);
  const float v = vb * denom;
  const float w = vc * denom;
  const glm::vec3 closest = a + ab * v + ac * w;
  const glm::vec3 d = p - closest;
  return glm::dot(d, d);
}

void AppendAssimpMesh(std::vector<MeshVertex>& out, const aiMesh& mesh, glm::vec3 fallbackColor) {
  if (!mesh.HasPositions() || mesh.mNumFaces == 0) {
    return;
  }
  // Assimp exposes raw C arrays; span keeps the indexing in bounds-checked C++.
  const std::span faces{mesh.mFaces, mesh.mNumFaces};
  const std::span vertices{mesh.mVertices, mesh.mNumVertices};
  for (const aiFace& tri : faces) {
    if (tri.mNumIndices < 3) {
      continue;
    }
    const std::span indices{tri.mIndices, tri.mNumIndices};
    const aiVector3D& pa = vertices[indices[0]];
    const aiVector3D& pb = vertices[indices[1]];
    const aiVector3D& pc = vertices[indices[2]];
    const glm::vec3 a{pa.x, pa.y, pa.z};
    const glm::vec3 b{pb.x, pb.y, pb.z};
    const glm::vec3 c{pc.x, pc.y, pc.z};
    glm::vec3 normal = glm::cross(b - a, c - a);
    const float length = glm::length(normal);
    if (length < 1.0e-12F) {
      continue;
    }
    normal /= length;
    glm::vec3 color = fallbackColor;
    if (mesh.HasVertexColors(0)) {
      const std::span colors{mesh.mColors[0], mesh.mNumVertices};
      const aiColor4D& ca = colors[indices[0]];
      color = {ca.r, ca.g, ca.b};
    }
    out.push_back({.position = a, .normal = normal, .color = color});
    out.push_back({.position = b, .normal = normal, .color = color});
    out.push_back({.position = c, .normal = normal, .color = color});
  }
}

}  // namespace

bool MeshPathLooksSupported(const std::filesystem::path& path) {
  const auto ext = LowerExtension(path);
  return ext == ".gltf" || ext == ".glb" || ext == ".obj" || ext == ".stl" || ext == ".fbx";
}

void FitMeshToTunnel(std::vector<MeshVertex>& mesh, const CarDomain& domain) {
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
  const float targetY = domainSize.y * 0.52F;
  const float targetX = domainSize.x * 0.44F;
  const float targetZ = domainSize.z * 0.58F;
  const float scale = std::min({targetY / extent.y, targetX / extent.x, targetZ / extent.z, 8.0F});
  const glm::vec3 center = 0.5F * (mn + mx);
  const glm::vec3 tunnelMid = domain.origin + 0.5F * domainSize;
  const float floorY = domain.origin.y + domain.cellSize * 1.25F;
  const glm::vec3 scaledMin = (mn - center) * scale;
  const glm::vec3 translation{tunnelMid.x - 0.18F * domainSize.x, floorY - scaledMin.y,
                              tunnelMid.z};
  for (MeshVertex& vertex : mesh) {
    vertex.position = (vertex.position - center) * scale + translation;
  }
}

std::vector<std::uint32_t> BuildWindOccupancy(const CarDomain& domain,
                                              std::span<const MeshVertex> mesh) {
  const std::uint32_t cellCount = domain.grid.x * domain.grid.y * domain.grid.z;
  std::vector<std::uint32_t> occupancy(cellCount, static_cast<std::uint32_t>(WindCell::Fluid));
  for (std::uint32_t z = 0; z < domain.grid.z; ++z) {
    for (std::uint32_t y = 0; y < domain.grid.y; ++y) {
      for (std::uint32_t x = 0; x < domain.grid.x; ++x) {
        const std::uint32_t index = x + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
        if (x == 0) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Inlet);
        } else if (x + 1 == domain.grid.x) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Outlet);
        } else if (y == 0 || y + 1 == domain.grid.y || z == 0 || z + 1 == domain.grid.z) {
          occupancy[index] = static_cast<std::uint32_t>(WindCell::Wall);
        }
      }
    }
  }

  const float reach = domain.cellSize * 0.78F;
  const float reachSq = reach * reach;
  for (std::size_t i = 0; i + 2 < mesh.size(); i += 3) {
    const glm::vec3 a = mesh[i].position;
    const glm::vec3 b = mesh[i + 1].position;
    const glm::vec3 c = mesh[i + 2].position;
    const glm::vec3 tmin = glm::min(a, glm::min(b, c));
    const glm::vec3 tmax = glm::max(a, glm::max(b, c));
    const glm::ivec3 c0 = glm::clamp(glm::ivec3((tmin - domain.origin) / domain.cellSize),
                                     glm::ivec3(0), glm::ivec3(domain.grid) - 1);
    const glm::ivec3 c1 = glm::clamp(glm::ivec3((tmax - domain.origin) / domain.cellSize),
                                     glm::ivec3(0), glm::ivec3(domain.grid) - 1);
    for (int z = c0.z; z <= c1.z; ++z) {
      for (int y = c0.y; y <= c1.y; ++y) {
        for (int x = c0.x; x <= c1.x; ++x) {
          const std::uint32_t index = static_cast<std::uint32_t>(x) +
                                      static_cast<std::uint32_t>(y) * domain.grid.x +
                                      static_cast<std::uint32_t>(z) * domain.grid.x * domain.grid.y;
          if (occupancy[index] != static_cast<std::uint32_t>(WindCell::Fluid)) {
            continue;
          }
          const glm::vec3 point = domain.origin + domain.cellSize * (glm::vec3(x, y, z) + 0.5F);
          if (DistPointTriangleSq(point, a, b, c) <= reachSq) {
            occupancy[index] = static_cast<std::uint32_t>(WindCell::Solid);
          }
        }
      }
    }
  }

  std::vector<std::uint8_t> reached(cellCount, 0);
  std::queue<std::uint32_t> queue;
  const auto tryEnqueue = [&](std::uint32_t x, std::uint32_t y, std::uint32_t z) {
    if (x >= domain.grid.x || y >= domain.grid.y || z >= domain.grid.z) {
      return;
    }
    const std::uint32_t index = x + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
    if (reached[index] != 0 || occupancy[index] != static_cast<std::uint32_t>(WindCell::Fluid)) {
      return;
    }
    reached[index] = 1;
    queue.push(index);
  };
  for (std::uint32_t z = 1; z + 1 < domain.grid.z; ++z) {
    for (std::uint32_t y = 1; y + 1 < domain.grid.y; ++y) {
      tryEnqueue(1, y, z);
    }
  }
  const std::array dx{1, -1, 0, 0, 0, 0};
  const std::array dy{0, 0, 1, -1, 0, 0};
  const std::array dz{0, 0, 0, 0, 1, -1};
  while (!queue.empty()) {
    const std::uint32_t index = queue.front();
    queue.pop();
    const std::uint32_t x = index % domain.grid.x;
    const std::uint32_t y = (index / domain.grid.x) % domain.grid.y;
    const std::uint32_t z = index / (domain.grid.x * domain.grid.y);
    for (int n = 0; n < 6; ++n) {
      tryEnqueue(
          static_cast<std::uint32_t>(static_cast<int>(x) + dx.at(static_cast<std::size_t>(n))),
          static_cast<std::uint32_t>(static_cast<int>(y) + dy.at(static_cast<std::size_t>(n))),
          static_cast<std::uint32_t>(static_cast<int>(z) + dz.at(static_cast<std::size_t>(n))));
    }
  }
  for (std::uint32_t i = 0; i < cellCount; ++i) {
    if (occupancy[i] == static_cast<std::uint32_t>(WindCell::Fluid) && reached[i] == 0) {
      occupancy[i] = static_cast<std::uint32_t>(WindCell::Solid);
    }
  }
  return occupancy;
}

std::vector<MeshVertex> LoadObstacleMesh(const std::filesystem::path& path) {
  if (!MeshPathLooksSupported(path)) {
    throw std::invalid_argument("mesh must be glTF, glb, OBJ, STL, or FBX");
  }
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      path.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                         aiProcess_GenSmoothNormals | aiProcess_PreTransformVertices |
                         aiProcess_ImproveCacheLocality);
  if (scene == nullptr || scene->mRootNode == nullptr) {
    throw std::runtime_error(std::string("failed to load mesh: ") + importer.GetErrorString());
  }
  std::vector<MeshVertex> mesh;
  mesh.reserve(4096);
  const glm::vec3 fallback{0.62F, 0.64F, 0.68F};
  const std::span parts{scene->mMeshes, scene->mNumMeshes};
  const std::span materials{scene->mMaterials, scene->mNumMaterials};
  for (const aiMesh* part : parts) {
    if (part == nullptr) {
      continue;
    }
    glm::vec3 color = fallback;
    if (part->mMaterialIndex < materials.size()) {
      aiColor4D diffuse;
      if (materials[part->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
        color = {diffuse.r, diffuse.g, diffuse.b};
      }
    }
    AppendAssimpMesh(mesh, *part, color);
  }
  if (mesh.size() < 3) {
    throw std::runtime_error("mesh has no triangles");
  }
  LogInfo("Loaded {} triangles from {}", mesh.size() / 3, path.string());
  return mesh;
}

std::filesystem::path DefaultFormulaCarPath() { return SPH_FORMULA_MESH_PATH; }

std::vector<MeshVertex> LoadDefaultFormulaCar(const CarDomain& domain) {
  auto mesh = LoadObstacleMesh(DefaultFormulaCarPath());
  FitMeshToTunnel(mesh, domain);
  return mesh;
}

glm::vec3 ColorFromCp(float cp, float cpMin, float cpMax) {
  const glm::vec3 blue{0.10F, 0.28F, 0.84F};
  const glm::vec3 white{0.97F, 0.97F, 0.98F};
  const glm::vec3 red{0.86F, 0.12F, 0.10F};
  if (cp >= 0.0F) {
    const float span = std::max(cpMax, 1.0e-4F);
    return glm::mix(white, red, std::clamp(cp / span, 0.0F, 1.0F));
  }
  const float span = std::min(cpMin, -1.0e-4F);
  return glm::mix(white, blue, std::clamp(cp / span, 0.0F, 1.0F));
}

namespace {

bool FluidOccupancy(std::uint32_t occupancy) {
  return occupancy == static_cast<std::uint32_t>(WindCell::Fluid) ||
         occupancy == static_cast<std::uint32_t>(WindCell::Inlet) ||
         occupancy == static_cast<std::uint32_t>(WindCell::Outlet);
}

float SampleDensity(const CarDomain& domain, std::span<const glm::vec4> macros,
                    std::span<const std::uint32_t> occupancy, glm::vec3 point) {
  const int gx = static_cast<int>(domain.grid.x);
  const int gy = static_cast<int>(domain.grid.y);
  const int gz = static_cast<int>(domain.grid.z);
  const auto densityAt = [&](int x, int y, int z) -> float {
    if (x < 0 || y < 0 || z < 0 || x >= gx || y >= gy || z >= gz) {
      return -1.0F;
    }
    const std::uint32_t index = static_cast<std::uint32_t>(x) +
                                static_cast<std::uint32_t>(y) * domain.grid.x +
                                static_cast<std::uint32_t>(z) * domain.grid.x * domain.grid.y;
    if (!FluidOccupancy(occupancy[index])) {
      return -1.0F;
    }
    return macros[index].x;
  };
  float sum = 0.0F;
  int count = 0;
  const glm::vec3 coord = (point - domain.origin) / domain.cellSize;
  const int x0 = static_cast<int>(std::floor(coord.x));
  const int y0 = static_cast<int>(std::floor(coord.y));
  const int z0 = static_cast<int>(std::floor(coord.z));
  for (int dz = -1; dz <= 1; ++dz) {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        const float density = densityAt(x0 + dx, y0 + dy, z0 + dz);
        if (density >= 0.0F) {
          sum += density;
          ++count;
        }
      }
    }
  }
  if (count == 0) {
    return -1.0F;
  }
  return sum / static_cast<float>(count);
}

}  // namespace

AeroPaint ColorMeshByPressure(std::vector<MeshVertex>& mesh, const CarDomain& domain,
                              std::span<const glm::vec4> macros,
                              std::span<const std::uint32_t> occupancy, float inletUx) {
  AeroPaint paint;
  if (mesh.size() < 3 || macros.empty() || occupancy.empty()) {
    return paint;
  }
  float rhoInf = 0.0F;
  int inletCount = 0;
  const std::uint32_t gy = domain.grid.y;
  const std::uint32_t gz = domain.grid.z;
  for (std::uint32_t z = 1; z + 1 < gz; ++z) {
    for (std::uint32_t y = 1; y + 1 < gy; ++y) {
      const std::uint32_t index = 1U + y * domain.grid.x + z * domain.grid.x * domain.grid.y;
      if (FluidOccupancy(occupancy[index])) {
        rhoInf += macros[index].x;
        ++inletCount;
      }
    }
  }
  if (inletCount == 0) {
    rhoInf = 1.0F;
  } else {
    rhoInf /= static_cast<float>(inletCount);
  }
  const float dynamic = std::max(1.5F * inletUx * inletUx, 1.0e-6F);
  std::vector<float> cps(mesh.size() / 3, 0.0F);
  float cpMin = 0.0F;
  float cpMax = 0.0F;
  bool any = false;
  for (std::size_t i = 0, face = 0; i + 2 < mesh.size(); i += 3, ++face) {
    const glm::vec3 a = mesh[i].position;
    const glm::vec3 b = mesh[i + 1].position;
    const glm::vec3 c = mesh[i + 2].position;
    const glm::vec3 centroid = (a + b + c) * (1.0F / 3.0F);
    glm::vec3 normal = mesh[i].normal;
    if (glm::dot(normal, normal) < 1.0e-8F) {
      normal = glm::cross(b - a, c - a);
    }
    const float length = glm::length(normal);
    if (length > 1.0e-8F) {
      normal /= length;
    } else {
      normal = {0.0F, 1.0F, 0.0F};
    }
    float density = -1.0F;
    for (int step = 1; step <= 4 && density < 0.0F; ++step) {
      const float offset = domain.cellSize * (0.45F * static_cast<float>(step));
      density = SampleDensity(domain, macros, occupancy, centroid + normal * offset);
      if (density < 0.0F) {
        density = SampleDensity(domain, macros, occupancy, centroid - normal * offset);
      }
    }
    if (density < 0.0F) {
      density = rhoInf;
    }
    const float cp = (density - rhoInf) / dynamic;
    cps[face] = std::isfinite(cp) ? cp : 0.0F;
    if (!any) {
      cpMin = cps[face];
      cpMax = cps[face];
      any = true;
    } else {
      cpMin = std::min(cpMin, cps[face]);
      cpMax = std::max(cpMax, cps[face]);
    }
  }
  std::vector<float> sorted = cps;
  std::ranges::sort(sorted);
  const std::size_t n = sorted.size();
  const float p05 = sorted[n * 5 / 100];
  const float p95 = sorted[n * 95 / 100];
  const float mag = std::max(
      {std::abs(p05), std::abs(p95), std::abs(cpMin) * 0.15F, std::abs(cpMax) * 0.15F, 0.25F});
  paint.cpMin = -mag;
  paint.cpMax = mag;
  for (std::size_t i = 0, face = 0; i + 2 < mesh.size(); i += 3, ++face) {
    const glm::vec3 color = ColorFromCp(cps[face], paint.cpMin, paint.cpMax);
    mesh[i].color = color;
    mesh[i + 1].color = color;
    mesh[i + 2].color = color;
  }
  return paint;
}

}  // namespace sph::sim
