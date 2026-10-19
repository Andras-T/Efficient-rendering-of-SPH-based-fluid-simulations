#include "app/ImageWriter.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

#include <cstring>
#include <format>
#include <stdexcept>
#include <vector>

namespace sph {

void WritePng(const std::filesystem::path& path, VkExtent2D extent,
              std::span<const std::uint8_t> rgba) {
  constexpr int Channels = 4;
  if (rgba.size() != std::size_t{extent.width} * extent.height * Channels) {
    throw std::invalid_argument("pixel buffer does not match the image size");
  }
  const auto width = static_cast<int>(extent.width);
  const auto height = static_cast<int>(extent.height);
  if (stbi_write_png(path.string().c_str(), width, height, Channels, rgba.data(),
                     width * Channels) == 0) {
    throw std::runtime_error(std::format("could not write {}", path.string()));
  }
}

std::vector<std::uint8_t> ReadPng(const std::filesystem::path& path, VkExtent2D& extent) {
  int width = 0;
  int height = 0;
  int channels = 0;
  stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
  if (pixels == nullptr || width <= 0 || height <= 0) {
    throw std::runtime_error(std::format("could not read {}", path.string()));
  }
  extent = {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
  const std::size_t byteCount =
      static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
  std::vector<std::uint8_t> rgba(byteCount);
  std::memcpy(rgba.data(), pixels, byteCount);
  stbi_image_free(pixels);
  return rgba;
}

}  // namespace sph
