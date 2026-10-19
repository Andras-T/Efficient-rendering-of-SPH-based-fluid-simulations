include(GoogleTest)

# CPU tests run everywhere; GPU tests need a Vulkan driver (lavapipe in CI).
add_executable(sph_cpu_tests OptionsTest.cpp)
target_link_libraries(sph_cpu_tests PRIVATE sph_engine GTest::gtest_main)
sph_set_warnings(sph_cpu_tests)
gtest_discover_tests(sph_cpu_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS cpu)

add_executable(sph_gpu_tests ComputeTest.cpp HeadlessRenderTest.cpp)
target_link_libraries(sph_gpu_tests PRIVATE sph_engine sph_shaders GTest::gtest_main)
sph_set_warnings(sph_gpu_tests)
gtest_discover_tests(sph_gpu_tests DISCOVERY_MODE PRE_TEST PROPERTIES LABELS gpu)
