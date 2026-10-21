add_library(sph_engine STATIC
  app/Headless.cpp
  app/ImageWriter.cpp
  app/ImGuiLayer.cpp
  app/Window.cpp
  app/Windowed.cpp
  core/Capture.cpp
  core/Log.cpp
  core/Options.cpp
  core/Simulation.cpp
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
  render/BlockPass.cpp
  render/Camera.cpp
  render/CarPass.cpp
  render/DebugView.cpp
  render/FluidPass.cpp
  render/FoamPass.cpp
  render/Hdri.cpp
  render/PaddlePass.cpp
  render/ParticlePass.cpp
  render/Renderer.cpp
  render/TankPass.cpp
  render/SkyPass.cpp
  render/SmokePass.cpp
  render/TonemapPass.cpp
  render/WindTracerPass.cpp
  render/WindVolumePass.cpp
  sim/Car.cpp
  sim/Mesh.cpp
  sim/CpuSolver.cpp
  sim/GpuParticles.cpp
  sim/Grid.cpp
  sim/Scene.cpp
  sim/SimParams.cpp
  sim/SmokeSolver.cpp
  sim/SphSolver.cpp
  sim/Tank.cpp
  sim/WindSolver.cpp
  ui/Hud.cpp)
target_include_directories(sph_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(sph_engine PUBLIC
  VK_NO_PROTOTYPES
  VMA_STATIC_VULKAN_FUNCTIONS=0
  VMA_DYNAMIC_VULKAN_FUNCTIONS=1
  GLFW_INCLUDE_NONE
  GLM_FORCE_DEPTH_ZERO_TO_ONE
  GLM_FORCE_RADIANS)
target_compile_definitions(sph_engine PRIVATE "SPH_HDRI_PATH=\"${SPH_HDRI_PATH}\"")
target_link_libraries(sph_engine
  PUBLIC volk::volk vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator glm::glm glfw
  PRIVATE imgui stb sph_shaders assimp::assimp)
sph_set_warnings(sph_engine)

add_executable(sph-fluid main.cpp)
target_link_libraries(sph-fluid PRIVATE sph_engine)
sph_set_warnings(sph-fluid)

include(GNUInstallDirs)
install(TARGETS sph-fluid RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
