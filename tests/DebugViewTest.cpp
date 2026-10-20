#include "core/Options.hpp"
#include "render/DebugView.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string_view>
#include <vector>

namespace sph {
namespace {

TEST(DebugViewTest, ParsesStageNames) {
  EXPECT_EQ(render::ParseDebugView("final"), render::DebugView::Final);
  EXPECT_EQ(render::ParseDebugView("particles"), render::DebugView::Particles);
  EXPECT_EQ(render::ParseDebugView("density"), render::DebugView::Density);
  EXPECT_EQ(render::ParseDebugView("velocity"), render::DebugView::Velocity);
  EXPECT_EQ(render::ParseDebugView("cell"), render::DebugView::Cell);
  EXPECT_EQ(render::ParseDebugView("depth"), render::DebugView::RawDepth);
  EXPECT_EQ(render::ParseDebugView("smooth"), render::DebugView::SmoothDepth);
  EXPECT_EQ(render::ParseDebugView("thickness"), render::DebugView::Thickness);
  EXPECT_EQ(render::ParseDebugView("normals"), render::DebugView::Normals);
  EXPECT_EQ(render::ParseDebugView("foam"), render::DebugView::Foam);
  EXPECT_THROW(
      {
        const auto parsed = render::ParseDebugView("bloom");
        (void)parsed;
      },
      std::invalid_argument);
}

TEST(DebugViewTest, CyclesThroughEveryStage) {
  EXPECT_EQ(render::NextDebugView(render::DebugView::Final), render::DebugView::Particles);
  EXPECT_EQ(render::NextDebugView(render::DebugView::Normals), render::DebugView::Foam);
  EXPECT_EQ(render::NextDebugView(render::DebugView::Foam), render::DebugView::Final);
  EXPECT_EQ(render::PrevDebugView(render::DebugView::Final), render::DebugView::Foam);
  EXPECT_EQ(render::PrevDebugView(render::DebugView::Particles), render::DebugView::Final);
  EXPECT_EQ(render::DebugViewCount, 10);
  EXPECT_EQ(render::NumericDebugViewKeys, 9);
  EXPECT_LT(render::NumericDebugViewKeys, render::DebugViewCount);
}

TEST(DebugViewTest, ClassifiesPasses) {
  EXPECT_TRUE(render::IsParticleView(render::DebugView::Density));
  EXPECT_FALSE(render::NeedsFluidPasses(render::DebugView::Particles));
  EXPECT_TRUE(render::NeedsFluidPasses(render::DebugView::Final));
  EXPECT_EQ(std::string_view(render::DebugViewName(render::DebugView::Thickness)), "thickness");
}

TEST(OptionsTest, ParsesView) {
  const std::vector<std::string_view> args{"--view", "normals"};
  EXPECT_EQ(ParseOptions(args).view, render::DebugView::Normals);
}

}  // namespace
}  // namespace sph
