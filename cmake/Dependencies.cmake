include(FetchContent)

set(SPH_DEPS_CACHE "${PROJECT_SOURCE_DIR}/.deps" CACHE PATH "Download cache for third-party archives")

# Archives are pinned by hash and cached in SPH_DEPS_CACHE, so reconfiguring works offline.
function(sph_declare name url hash)
  get_filename_component(fileName "${url}" NAME)
  FetchContent_Declare(${name}
    URL "${url}"
    URL_HASH SHA256=${hash}
    DOWNLOAD_DIR "${SPH_DEPS_CACHE}"
    DOWNLOAD_NAME "${name}-${fileName}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM)
endfunction()

set(github https://github.com)

sph_declare(vulkan_headers
  ${github}/KhronosGroup/Vulkan-Headers/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz
  4a078be12bef21cfebc09d878b77a63cff9d68f899254a0b00d0e37ef73e7f7e)
sph_declare(volk
  ${github}/zeux/volk/archive/refs/tags/vulkan-sdk-1.4.363.0.tar.gz
  1547d8d74395d4048fb3f4a6313da56db626e89b55db238d0d7e8944c8a645f3)
sph_declare(vk_bootstrap
  ${github}/charles-lunarg/vk-bootstrap/archive/refs/tags/v1.4.363.tar.gz
  11927b43a5c5d64646982b273d6436d2243acaa2da8834f7ba382060431155fe)
sph_declare(vma
  ${github}/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator/archive/refs/tags/v3.4.0.tar.gz
  822aa850c6ce77346ae96a8a1d351d52e77e85929f35363849a0a4e638e0a2a1)
sph_declare(glfw
  ${github}/glfw/glfw/archive/refs/tags/3.5.1.tar.gz
  5234f4f29473e9a06bc7847d8371858dd135d38466eeeaa652fdc9f8f9ff0c20)
sph_declare(glm
  ${github}/g-truc/glm/archive/refs/tags/1.0.3.tar.gz
  6775e47231a446fd086d660ecc18bcd076531cfedd912fbd66e576b118607001)
sph_declare(imgui
  ${github}/ocornut/imgui/archive/refs/tags/v1.93.0.tar.gz
  f7bf6bc76e5d79d40d751096663e6c5d18daf08129dd52e7f7b09e4251c8eeee)
sph_declare(stb
  ${github}/nothings/stb/archive/2c980bb59875b0d32144a71867fbdebb2f77cd20.tar.gz
  9a955b1b49a4410088a2e0ee2a9c057c3c907d0c1d75454144cb980aca0ba515)

set(VOLK_PULL_IN_VULKAN OFF)
set(VK_BOOTSTRAP_TEST OFF)
set(VK_BOOTSTRAP_INSTALL OFF)
set(VMA_ENABLE_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_INSTALL OFF)
set(GLM_BUILD_TESTS OFF)
set(GLM_BUILD_INSTALL OFF)
set(GLM_BUILD_LIBRARY OFF)

FetchContent_MakeAvailable(vulkan_headers volk vk_bootstrap vma glfw glm imgui stb)

target_link_libraries(volk PUBLIC Vulkan::Headers)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui SYSTEM PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(imgui PUBLIC IMGUI_IMPL_VULKAN_USE_VOLK GLFW_INCLUDE_NONE)
target_link_libraries(imgui PUBLIC volk::volk glfw)

add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${stb_SOURCE_DIR})

if(SPH_BUILD_TESTS)
  sph_declare(googletest
    ${github}/google/googletest/archive/refs/tags/v1.18.0.tar.gz
    6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5)
  set(BUILD_GMOCK OFF)
  set(INSTALL_GTEST OFF)
  set(gtest_force_shared_crt ON)
  FetchContent_MakeAvailable(googletest)
endif()

# glslang is built from the pinned source so every platform compiles shaders the same way.
set(SPH_GLSLANG "" CACHE FILEPATH "glslang executable to use instead of building the pinned one")
if(SPH_GLSLANG)
  set(SPH_GLSLANG_EXE ${SPH_GLSLANG})
else()
  sph_declare(glslang
    ${github}/KhronosGroup/glslang/archive/refs/tags/16.6.0.tar.gz
    9c09b901149c729df745057dafa815278aaa101b84d2b6e14f16a42de52f97f2)
  set(ENABLE_OPT OFF)
  set(ENABLE_HLSL OFF)
  set(ENABLE_GLSLANG_JS OFF)
  set(GLSLANG_TESTS OFF)
  set(GLSLANG_ENABLE_INSTALL OFF)
  set(BUILD_EXTERNAL OFF)
  FetchContent_MakeAvailable(glslang)
  set(SPH_GLSLANG_EXE $<TARGET_FILE:glslang-standalone>)
  set(SPH_GLSLANG_TARGET glslang-standalone)
endif()
