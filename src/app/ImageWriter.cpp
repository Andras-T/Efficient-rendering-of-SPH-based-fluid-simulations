#include "app/ImageWriter.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <format>
#include <stdexcept>

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

}  // namespace sph
