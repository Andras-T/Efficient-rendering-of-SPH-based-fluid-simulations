set(SPH_EMBED_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/EmbedSpirv.cmake)

# Compiles GLSL shaders to SPIR-V and embeds them in a static library.
# Each shader is looked up at runtime by its file name, e.g. "Background.vert".
function(sph_add_shader_library target)
  cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;INCLUDES")
  set(outDir ${CMAKE_CURRENT_BINARY_DIR}/spirv)
  file(MAKE_DIRECTORY ${outDir})

  set(includes "")
  set(arrays "")
  set(entries "")
  foreach(source ${arg_SOURCES})
    get_filename_component(name ${source} NAME)
    set(spv ${outDir}/${name}.spv)
    set(inc ${outDir}/${name}.inc)
    add_custom_command(
      OUTPUT ${spv} ${inc}
      COMMAND ${SPH_GLSLANG_EXE} -V --target-env vulkan1.3 --quiet
        -I${CMAKE_CURRENT_SOURCE_DIR} --depfile ${spv}.d -o ${spv}
        ${CMAKE_CURRENT_SOURCE_DIR}/${source}
      COMMAND ${CMAKE_COMMAND} -DINPUT=${spv} -DOUTPUT=${inc} -P ${SPH_EMBED_SCRIPT}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${source} ${arg_INCLUDES} ${SPH_EMBED_SCRIPT}
        ${SPH_GLSLANG_TARGET}
      DEPFILE ${spv}.d
      COMMENT "Compiling shader ${source}"
      VERBATIM)
    list(APPEND includes ${inc})
    string(MAKE_C_IDENTIFIER "Spirv_${name}" array)
    string(APPEND arrays "constexpr std::uint32_t ${array}[] = {\n#include \"${name}.inc\"\n};\n")
    string(APPEND entries "    Entry{\"${name}\", ${array}},\n")
  endforeach()

  set(registry ${outDir}/ShaderLibrary.cpp)
  file(CONFIGURE OUTPUT ${registry} @ONLY CONTENT [=[
#include "ShaderLibrary.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {

@arrays@
struct Entry {
  std::string_view name;
  std::span<const std::uint32_t> code;
};

constexpr std::array Entries{
@entries@};

}  // namespace

std::span<const std::uint32_t> sph::shaders::Find(std::string_view name) {
  const auto it = std::ranges::find(Entries, name, &Entry::name);
  return it == Entries.end() ? std::span<const std::uint32_t>{} : it->code;
}
]=])

  add_library(${target} STATIC ${registry} ${includes})
  target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${outDir})
endfunction()
