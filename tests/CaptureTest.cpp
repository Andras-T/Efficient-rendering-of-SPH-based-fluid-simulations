#include "core/Capture.hpp"

#include <gtest/gtest.h>

namespace sph {
namespace {

TEST(CaptureTest, SequenceNamesAreZeroPadded) {
  EXPECT_EQ(SequenceFramePath("frames", 0), std::filesystem::path("frames") / "0000.png");
  EXPECT_EQ(SequenceFramePath("frames", 12), std::filesystem::path("frames") / "0012.png");
}

}  // namespace
}  // namespace sph
