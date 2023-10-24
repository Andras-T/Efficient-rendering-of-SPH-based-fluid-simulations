cmake_minimum_required(VERSION 3.21)
project(FluidSimulationSPH)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_VERBOSE_MAKEFILE OFF)

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

set(IMGUI_DIR lib/imgui)
set(STB_DIR lib/stb)
set(VULKAN src/Vulkan)

set(SOURCES
        ${VULKAN}/BufferManager/BufferManager.cpp
        ${VULKAN}/CommandPool/CommandPoolManager.cpp
        ${VULKAN}/DescriptorManager/DescriptorManager.cpp
        ${VULKAN}/DeviceManager/DeviceManager.cpp
        ${VULKAN}/PipelineManager/PipelineManager.cpp
        ${VULKAN}/PipelineManager/Pipeline.cpp
        ${VULKAN}/Render/Render.cpp
        ${VULKAN}/SwapchainManager/SwapchainManager.cpp
        ${VULKAN}/SwapchainManager/Image.cpp
        ${VULKAN}/Utils/Utils.cpp
        ${VULKAN}/VulkanObject/VulkanObject.cpp
        ${VULKAN}/Window/Window.cpp
        ${VULKAN}/Render/ImGuiRender.cpp
        ${VULKAN}/FluidInstance.cpp
        src/FluidSimulationEnvironment.cpp
        src/Logger.cpp
        src/main.cpp

        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp 
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
        ${IMGUI_DIR}/imgui.cpp 
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp 
        ${IMGUI_DIR}/imgui_tables.cpp 
        ${IMGUI_DIR}/imgui_widgets.cpp

        ${STB_DIR}/stb_image.cpp 
        ${STB_DIR}/stb_image.h)

add_executable(FluidSimulationSPH ${SOURCES} res/app.o)

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