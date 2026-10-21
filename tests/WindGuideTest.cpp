#include "ui/WindGuide.hpp"

#include <gtest/gtest.h>

#include <string_view>
#include <unordered_set>

namespace sph::ui {
namespace {

TEST(WindGuideTest, WalksEveryPipelineStage) {
  EXPECT_EQ(WindGuideStepCount(), 6U);
  std::unordered_set<std::string_view> titles;
  std::unordered_set<std::uint32_t> looks;
  for (std::uint32_t i = 0; i < WindGuideStepCount(); ++i) {
    const WindGuideStep& step = WindGuideAt(i);
    EXPECT_FALSE(std::string_view(step.title).empty());
    EXPECT_FALSE(std::string_view(step.text).empty());
    titles.insert(step.title);
    looks.insert((static_cast<std::uint32_t>(step.view) << 3U) | (step.showCar ? 4U : 0U) |
                 (step.showTracers ? 2U : 0U) | (step.showVolume ? 1U : 0U));
  }
  EXPECT_EQ(titles.size(), WindGuideStepCount());
  EXPECT_EQ(looks.size(), WindGuideStepCount());
  EXPECT_EQ(WindGuideAt(0).view, render::DebugView::Final);
  EXPECT_FALSE(WindGuideAt(0).showCar);
  EXPECT_FALSE(WindGuideAt(0).showTracers);
  EXPECT_FALSE(WindGuideAt(0).showVolume);
  EXPECT_EQ(WindGuideAt(WindGuideStepCount() - 1U).view, render::DebugView::Final);
  EXPECT_TRUE(WindGuideAt(WindGuideStepCount() - 1U).showCar);
  EXPECT_FALSE(WindGuideAt(WindGuideStepCount() - 1U).showTracers);
  EXPECT_TRUE(WindGuideAt(WindGuideStepCount() - 1U).showVolume);
}

TEST(WindGuideTest, ContinueAdvancesThenFinishes) {
  std::uint32_t step = 0;
  bool active = true;
  EXPECT_TRUE(ContinueWindGuide(step, active));
  EXPECT_EQ(step, 1U);
  EXPECT_TRUE(active);
  step = WindGuideStepCount() - 1U;
  EXPECT_FALSE(ContinueWindGuide(step, active));
  EXPECT_FALSE(active);
}

TEST(WindGuideTest, SelectClampsAndActivates) {
  std::uint32_t step = 0;
  bool active = false;
  SelectWindGuideStep(step, active, 3);
  EXPECT_TRUE(active);
  EXPECT_EQ(step, 3U);
  EXPECT_EQ(WindGuideAt(step).view, render::DebugView::Velocity);
  EXPECT_TRUE(WindGuideAt(step).showCar);
  SelectWindGuideStep(step, active, 99);
  EXPECT_EQ(step, WindGuideStepCount() - 1U);
}

}  // namespace
}  // namespace sph::ui
