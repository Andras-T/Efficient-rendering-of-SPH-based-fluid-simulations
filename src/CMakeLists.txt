add_library(sph_engine STATIC
  app/Headless.cpp
  app/ImageWriter.cpp
  app/ImGuiLayer.cpp
  app/Window.cpp
  app/Windowed.cpp
  core/Capture.cpp
  core/Log.cpp
  core/Options.cpp
  gpu/Buffer.cpp
  gpu/Commands.cpp
  gpu/Context.cpp
  gpu/Handles.cpp
  gpu/Image.cpp
  gpu/Pipeline.cpp
  gpu/Swapchain.cpp
  gpu/Timestamps.cpp
  gpu/Vulkan.cpp
  render/BackgroundPass.cpp
  render/Camera.cpp
  render/ParticlePass.cpp
  render/Renderer.cpp
  sim/GpuParticles.cpp
  sim/Scene.cpp
  sim/SimParams.cpp
  ui/Hud.cpp)
target_include_directories(sph_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(sph_engine PUBLIC
  VK_NO_PROTOTYPES
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
  GLFW_INCLUDE_NONE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS)
target_link_libraries(sph_engine
  PUBLIC volk::volk vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator glm::glm glfw
  PRIVATE imgui stb sph_shaders)
sph_set_warnings(sph_engine)

add_executable(sph-fluid main.cpp)
target_link_libraries(sph-fluid PRIVATE sph_engine)
sph_set_warnings(sph-fluid)
