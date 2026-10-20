#include "ui/FluidGuide.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <unordered_set>

namespace sph::ui {
namespace {

TEST(FluidGuideTest, WalksEveryPipelineStage) {
  EXPECT_EQ(FluidGuideStepCount(), 10U);
  std::unordered_set<render::DebugView> views;
  std::unordered_set<std::string_view> titles;
  for (std::uint32_t i = 0; i < FluidGuideStepCount(); ++i) {
    const FluidGuideStep& step = FluidGuideAt(i);
    EXPECT_FALSE(std::string_view(step.title).empty());
    EXPECT_FALSE(std::string_view(step.text).empty());
    views.insert(step.view);
    titles.insert(step.title);
  }
  EXPECT_EQ(views.size(), FluidGuideStepCount());
  EXPECT_EQ(titles.size(), FluidGuideStepCount());
  EXPECT_EQ(FluidGuideAt(0).view, render::DebugView::Particles);
  EXPECT_EQ(FluidGuideAt(FluidGuideStepCount() - 1U).view, render::DebugView::Final);
}

TEST(FluidGuideTest, ContinueAdvancesThenFinishes) {
  std::uint32_t step = 0;
  bool active = true;
  EXPECT_TRUE(ContinueFluidGuide(step, active));
  EXPECT_EQ(step, 1U);
  EXPECT_TRUE(active);
  step = FluidGuideStepCount() - 1U;
  EXPECT_FALSE(ContinueFluidGuide(step, active));
  EXPECT_FALSE(active);
}

TEST(FluidGuideTest, SelectClampsAndActivates) {
  std::uint32_t step = 0;
  bool active = false;
  SelectFluidGuideStep(step, active, 3);
  EXPECT_TRUE(active);
  EXPECT_EQ(step, 3U);
  EXPECT_EQ(FluidGuideAt(step).view, render::DebugView::Velocity);
  SelectFluidGuideStep(step, active, 99);
  EXPECT_EQ(step, FluidGuideStepCount() - 1U);
}

}  // namespace
}  // namespace sph::ui
