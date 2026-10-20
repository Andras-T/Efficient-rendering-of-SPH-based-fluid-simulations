#include "core/Options.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace sph {
namespace {

Options Parse(std::initializer_list<std::string_view> args) {
  const std::vector<std::string_view> list(args);
  return ParseOptions(list);
}

TEST(OptionsTest, DefaultsToWindowedRunUntilClosed) {
  const Options options = Parse({});
  EXPECT_FALSE(options.headless);
  EXPECT_EQ(options.frames, 0U);
  EXPECT_EQ(options.width, 1280U);
  EXPECT_EQ(options.height, 720U);
  EXPECT_TRUE(options.capturePath.empty());
}

TEST(OptionsTest, HeadlessRendersOneFrameByDefault) {
  EXPECT_EQ(Parse({"--headless"}).frames, 1U);
  EXPECT_EQ(Parse({"--headless", "--frames", "5"}).frames, 5U);
}

TEST(OptionsTest, CaptureImpliesHeadless) {
  const Options options = Parse({"--capture", "out.png"});
  EXPECT_TRUE(options.headless);
  EXPECT_EQ(options.capturePath, "out.png");
}

TEST(OptionsTest, ParsesSolver) {
  EXPECT_EQ(Parse({"--solver", "dfsph"}).solver, sim::SolverMethod::Dfsph);
  EXPECT_EQ(Parse({"--solver", "wcsph"}).solver, sim::SolverMethod::Wcsph);
  EXPECT_THROW(Parse({"--solver", "pbf"}), std::invalid_argument);
}

TEST(OptionsTest, ParsesScenePreset) {
  EXPECT_EQ(Parse({"--scene", "drop"}).scene, sim::ScenePreset::Drop);
  EXPECT_EQ(Parse({"--scene", "double"}).scene, sim::ScenePreset::DoubleDam);
  EXPECT_EQ(Parse({"--scene", "empty"}).scene, sim::ScenePreset::Empty);
  EXPECT_EQ(Parse({"--scene", "dropin"}).scene, sim::ScenePreset::DropIn);
  EXPECT_THROW(Parse({"--scene", "plasma"}), std::invalid_argument);
}

TEST(OptionsTest, ParsesTankShape) {
  EXPECT_EQ(Parse({"--tank", "cylinder"}).tank, sim::TankShape::Cylinder);
  EXPECT_EQ(Parse({"--tank", "hexagon"}).tank, sim::TankShape::Hexagon);
  EXPECT_EQ(Parse({"--tank", "cube"}).tank, sim::TankShape::Box);
  EXPECT_THROW(Parse({"--tank", "torus"}), std::invalid_argument);
}

TEST(OptionsTest, SequenceImpliesHeadless) {
  const Options options = Parse({"--sequence", "frames", "--frames", "4"});
  EXPECT_TRUE(options.headless);
  EXPECT_EQ(options.sequenceDirectory, "frames");
  EXPECT_EQ(options.frames, 4U);
}

TEST(OptionsTest, ParsesSize) {
  const Options options = Parse({"--size", "640x480"});
  EXPECT_EQ(options.width, 640U);
  EXPECT_EQ(options.height, 480U);
}

TEST(OptionsTest, ValidationFlagsOverrideDefault) {
  EXPECT_TRUE(Parse({"--validation"}).validation);
  EXPECT_FALSE(Parse({"--validation", "--no-validation"}).validation);
}

TEST(OptionsTest, RejectsMalformedInput) {
  EXPECT_THROW(Parse({"--bogus"}), std::invalid_argument);
  EXPECT_THROW(Parse({"--frames"}), std::invalid_argument);
  EXPECT_THROW(Parse({"--frames", "ten"}), std::invalid_argument);
  EXPECT_THROW(Parse({"--frames", "-1"}), std::invalid_argument);
  EXPECT_THROW(Parse({"--size", "640"}), std::invalid_argument);
  EXPECT_THROW(Parse({"--size", "0x480"}), std::invalid_argument);
  EXPECT_THROW(Parse({"--size", "640x480x2"}), std::invalid_argument);
}

}  // namespace
}  // namespace sph
