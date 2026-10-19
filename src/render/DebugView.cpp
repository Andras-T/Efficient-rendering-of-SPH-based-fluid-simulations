#include "render/DebugView.hpp"

#include <format>
#include <stdexcept>

namespace sph::render {

const char* DebugViewName(DebugView view) {
  switch (view) {
    case DebugView::Final:
      return "final";
    case DebugView::Particles:
      return "particles";
    case DebugView::Density:
      return "density";
    case DebugView::Velocity:
      return "velocity";
    case DebugView::Cell:
      return "cell";
    case DebugView::RawDepth:
      return "depth";
    case DebugView::SmoothDepth:
      return "smooth";
    case DebugView::Thickness:
      return "thickness";
    case DebugView::Normals:
      return "normals";
  }
  return "final";
}

DebugView ParseDebugView(std::string_view name) {
  if (name == "final") {
    return DebugView::Final;
  }
  if (name == "particles") {
    return DebugView::Particles;
  }
  if (name == "density") {
    return DebugView::Density;
  }
  if (name == "velocity") {
    return DebugView::Velocity;
  }
  if (name == "cell") {
    return DebugView::Cell;
  }
  if (name == "depth" || name == "raw") {
    return DebugView::RawDepth;
  }
  if (name == "smooth") {
    return DebugView::SmoothDepth;
  }
  if (name == "thickness") {
    return DebugView::Thickness;
  }
  if (name == "normals") {
    return DebugView::Normals;
  }
  throw std::invalid_argument(std::format("unknown view '{}'", name));
}

DebugView NextDebugView(DebugView view) {
  const auto index =
      static_cast<std::uint8_t>((static_cast<std::uint8_t>(view) + 1U) % DebugViewCount);
  return static_cast<DebugView>(index);
}

DebugView PrevDebugView(DebugView view) {
  const auto index = static_cast<std::uint8_t>(
      (static_cast<std::uint8_t>(view) + DebugViewCount - 1U) % DebugViewCount);
  return static_cast<DebugView>(index);
}

bool IsParticleView(DebugView view) {
  return view == DebugView::Particles || view == DebugView::Density ||
         view == DebugView::Velocity || view == DebugView::Cell;
}

bool NeedsFluidPasses(DebugView view) { return !IsParticleView(view); }

}  // namespace sph::render
