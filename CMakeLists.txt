cmake_minimum_required(VERSION 3.21)
project(FluidSimulationSPH)

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

set(IMGUI_DIR lib/imgui)
set(STB_DIR lib/stb)

set(SOURCES
        src/Vulkan/BufferManager/BufferManager.cpp
        src/Vulkan/CommandPool/CommandPoolManager.cpp
        src/Vulkan/DescriptorManager/DescriptorManager.cpp
        src/Vulkan/DeviceManager/DeviceManager.cpp
        src/FluidSimulationEnvironment.cpp
        src/main.cpp
        src/Vulkan/PipelineManager/PipelineManager.cpp
        src/Vulkan/Render/Render.cpp
        src/Vulkan/SwapchainManager/SwapchainManager.cpp
        src/Vulkan/Utils/Utils.cpp
        src/Vulkan/VulkanObject/VulkanObject.cpp
        src/Vulkan/Window/Window.cpp
        src/Vulkan/Render/ImGuiRender.cpp
        src/Vulkan/FluidInstance.cpp

        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui.h ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp ${IMGUI_DIR}/imgui_tables.cpp ${IMGUI_DIR}/imgui_widgets.cpp

        ${STB_DIR}/stb_image.cpp ${STB_DIR}/stb_image.h)

add_executable(FluidSimulationSPH ${SOURCES})

FILE(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/shaders")

ADD_CUSTOM_TARGET(
        shadersTarget
        COMMAND cmd /c start ${CMAKE_SOURCE_DIR}/shaders/compile.bat
)

ADD_DEPENDENCIES(FluidSimulationSPH shadersTarget)

find_package(Vulkan REQUIRED)
add_subdirectory("lib/glfw")
add_subdirectory("lib/glm")

include_directories(${IMGUI_DIR} ${IMGUI_DIR}/backends ${STB_DIR})

target_link_libraries(FluidSimulationSPH Vulkan::Vulkan glm glfw)