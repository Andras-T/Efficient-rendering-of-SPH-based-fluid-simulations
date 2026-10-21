include(GoogleTest)

# CPU tests run everywhere; GPU tests need a Vulkan driver (lavapipe in CI).
add_executable(sph_cpu_tests OptionsTest.cpp CameraTest.cpp CaptureTest.cpp SceneTest.cpp
               KernelTest.cpp GridTest.cpp CpuSolverTest.cpp DebugViewTest.cpp CarTest.cpp
               SmokeTest.cpp FluidGuideTest.cpp WindGuideTest.cpp SmokeGuideTest.cpp
               MeshTest.cpp)
target_link_libraries(sph_cpu_tests PRIVATE sph_engine GTest::gtest_main)
sph_set_warnings(sph_cpu_tests)
# Windows AV/cold-start can exceed the default 5s discovery timeout.
gtest_discover_tests(sph_cpu_tests DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60
                     PROPERTIES LABELS cpu)

add_executable(sph_gpu_tests ComputeTest.cpp HeadlessRenderTest.cpp TimestampTest.cpp
               ParticleRenderTest.cpp GpuSphTest.cpp GoldenImageTest.cpp GpuWindTest.cpp
               GpuSmokeTest.cpp)
target_link_libraries(sph_gpu_tests PRIVATE sph_engine sph_shaders GTest::gtest_main)
sph_set_warnings(sph_gpu_tests)
target_compile_definitions(sph_gpu_tests PRIVATE
  "SPH_GOLDEN_DIR=\"${PROJECT_SOURCE_DIR}/tests/goldens\"")
if(SPH_WRITE_GOLDENS)
  target_compile_definitions(sph_gpu_tests PRIVATE SPH_WRITE_GOLDENS)
endif()
gtest_discover_tests(sph_gpu_tests DISCOVERY_MODE PRE_TEST DISCOVERY_TIMEOUT 60
                     PROPERTIES LABELS gpu)
