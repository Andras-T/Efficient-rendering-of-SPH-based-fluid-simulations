#include "ui/SmokeGuide.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <unordered_set>

namespace sph::ui {
namespace {

TEST(SmokeGuideTest, WalksEveryPipelineStage) {
  EXPECT_EQ(SmokeGuideStepCount(), 6U);
  std::unordered_set<std::string_view> titles;
  std::unordered_set<std::uint32_t> looks;
  for (std::uint32_t i = 0; i < SmokeGuideStepCount(); ++i) {
    const SmokeGuideStep& step = SmokeGuideAt(i);
    EXPECT_FALSE(std::string_view(step.title).empty());
    EXPECT_FALSE(std::string_view(step.text).empty());
    titles.insert(step.title);
    const std::uint32_t clip = step.clipY > 0.0F ? 1U : 0U;
    looks.insert((static_cast<std::uint32_t>(step.view) << 2U) | (step.showVolume ? 2U : 0U) |
                 clip);
  }
  EXPECT_EQ(titles.size(), SmokeGuideStepCount());
  EXPECT_EQ(looks.size(), SmokeGuideStepCount());
  EXPECT_FALSE(SmokeGuideAt(0).showVolume);
  EXPECT_TRUE(SmokeGuideAt(1).showVolume);
  EXPECT_GT(SmokeGuideAt(1).clipY, 0.0F);
  EXPECT_EQ(SmokeGuideAt(SmokeGuideStepCount() - 1U).view, render::DebugView::Final);
  EXPECT_TRUE(SmokeGuideAt(SmokeGuideStepCount() - 1U).showVolume);
  EXPECT_EQ(SmokeGuideAt(SmokeGuideStepCount() - 1U).clipY, 0.0F);
}

TEST(SmokeGuideTest, ContinueAdvancesThenFinishes) {
  std::uint32_t step = 0;
  bool active = true;
  EXPECT_TRUE(ContinueSmokeGuide(step, active));
  EXPECT_EQ(step, 1U);
  EXPECT_TRUE(active);
  step = SmokeGuideStepCount() - 1U;
  EXPECT_FALSE(ContinueSmokeGuide(step, active));
  EXPECT_FALSE(active);
}

TEST(SmokeGuideTest, SelectClampsAndActivates) {
  std::uint32_t step = 0;
  bool active = false;
  SelectSmokeGuideStep(step, active, 4);
  EXPECT_TRUE(active);
  EXPECT_EQ(step, 4U);
  EXPECT_EQ(SmokeGuideAt(step).view, render::DebugView::Velocity);
  SelectSmokeGuideStep(step, active, 99);
  EXPECT_EQ(step, SmokeGuideStepCount() - 1U);
}

}  // namespace
}  // namespace sph::ui
