#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sph::test {

inline void WriteBytes(const std::filesystem::path& path, std::string_view bytes) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to write " + path.string());
  }
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

inline std::filesystem::path WriteCubeObj(const std::filesystem::path& directory) {
  const auto path = directory / "cube.obj";
  WriteBytes(path, R"(o cube
v -1 0 -1
v 1 0 -1
v 1 1 -1
v -1 1 -1
v -1 0 1
v 1 0 1
v 1 1 1
v -1 1 1
f 1 2 3
f 1 3 4
f 5 8 7
f 5 7 6
f 1 5 6
f 1 6 2
f 4 3 7
f 4 7 8
f 1 4 8
f 1 8 5
f 2 6 7
f 2 7 3
)");
  return path;
}

inline std::filesystem::path WriteCubeStl(const std::filesystem::path& directory) {
  const auto path = directory / "cube.stl";
  WriteBytes(path, R"(solid cube
facet normal 0 0 -1
 outer loop
  vertex -1 0 -1
  vertex 1 0 -1
  vertex 1 1 -1
 endloop
endfacet
facet normal 0 0 -1
 outer loop
  vertex -1 0 -1
  vertex 1 1 -1
  vertex -1 1 -1
 endloop
endfacet
facet normal 0 0 1
 outer loop
  vertex -1 0 1
  vertex -1 1 1
  vertex 1 1 1
 endloop
endfacet
facet normal 0 0 1
 outer loop
  vertex -1 0 1
  vertex 1 1 1
  vertex 1 0 1
 endloop
endfacet
facet normal 0 -1 0
 outer loop
  vertex -1 0 -1
  vertex -1 0 1
  vertex 1 0 1
 endloop
endfacet
facet normal 0 -1 0
 outer loop
  vertex -1 0 -1
  vertex 1 0 1
  vertex 1 0 -1
 endloop
endfacet
facet normal 0 1 0
 outer loop
  vertex -1 1 -1
  vertex 1 1 -1
  vertex 1 1 1
 endloop
endfacet
facet normal 0 1 0
 outer loop
  vertex -1 1 -1
  vertex 1 1 1
  vertex -1 1 1
 endloop
endfacet
facet normal -1 0 0
 outer loop
  vertex -1 0 -1
  vertex -1 1 -1
  vertex -1 1 1
 endloop
endfacet
facet normal -1 0 0
 outer loop
  vertex -1 0 -1
  vertex -1 1 1
  vertex -1 0 1
 endloop
endfacet
facet normal 1 0 0
 outer loop
  vertex 1 0 -1
  vertex 1 0 1
  vertex 1 1 1
 endloop
endfacet
facet normal 1 0 0
 outer loop
  vertex 1 0 -1
  vertex 1 1 1
  vertex 1 1 -1
 endloop
endfacet
endsolid cube
)");
  return path;
}

inline std::filesystem::path WriteCubeGltf(const std::filesystem::path& directory) {
  const auto binPath = directory / "cube.bin";
  const auto gltfPath = directory / "cube.gltf";
  constexpr std::array<float, 36> Positions{
      -1.0F, 0.0F, -1.0F, 1.0F,  0.0F, -1.0F, 1.0F,  1.0F, -1.0F, -1.0F, 0.0F, -1.0F,
      1.0F,  1.0F, -1.0F, -1.0F, 1.0F, -1.0F, -1.0F, 0.0F, 1.0F,  -1.0F, 1.0F, 1.0F,
      1.0F,  1.0F, 1.0F,  -1.0F, 0.0F, 1.0F,  1.0F,  1.0F, 1.0F,  1.0F,  0.0F, 1.0F,
  };
  const std::string bytes(reinterpret_cast<const char*>(Positions.data()),
                          Positions.size() * sizeof(float));
  WriteBytes(binPath, bytes);
  WriteBytes(gltfPath, R"({
  "asset": {"version": "2.0"},
  "scenes": [{"nodes": [0]}],
  "nodes": [{"mesh": 0}],
  "meshes": [{"primitives": [{"attributes": {"POSITION": 0}}]}],
  "accessors": [{
    "bufferView": 0,
    "componentType": 5126,
    "count": 12,
    "type": "VEC3",
    "max": [1.0, 1.0, 1.0],
    "min": [-1.0, 0.0, -1.0]
  }],
  "bufferViews": [{"buffer": 0, "byteLength": 144}],
  "buffers": [{"byteLength": 144, "uri": "cube.bin"}]
})");
  return gltfPath;
}

}  // namespace sph::test
