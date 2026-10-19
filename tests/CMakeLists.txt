include(GoogleTest)

# CPU tests run everywhere; GPU tests need a Vulkan driver (lavapipe in CI).
add_executable(sph_cpu_tests OptionsTest.cpp CameraTest.cpp CaptureTest.cpp SceneTest.cpp
               KernelTest.cpp GridTest.cpp CpuSolverTest.cpp DebugViewTest.cpp)
target_link_libraries(sph_cpu_tests PRIVATE sph_engine GTest::gtest_main)
sph_set_warnings(sph_cpu_tests)
gtest_discover_tests(sph_cpu_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS cpu)

add_executable(sph_gpu_tests ComputeTest.cpp HeadlessRenderTest.cpp TimestampTest.cpp
               ParticleRenderTest.cpp GpuSphTest.cpp GoldenImageTest.cpp)
target_link_libraries(sph_gpu_tests PRIVATE sph_engine sph_shaders GTest::gtest_main)
sph_set_warnings(sph_gpu_tests)
target_compile_definitions(sph_gpu_tests PRIVATE
  "SPH_GOLDEN_DIR=\"${PROJECT_SOURCE_DIR}/tests/goldens\"")
gtest_discover_tests(sph_gpu_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS gpu)
