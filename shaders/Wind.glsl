#ifndef SPH_WIND_GLSL
#define SPH_WIND_GLSL

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference2 : require

const uint WindFluid = 0;
const uint WindSolid = 1;
const uint WindInlet = 2;
const uint WindOutlet = 3;
const uint WindWall = 4;
const uint LbmQ = 19;

struct LbmPop {
  float f[20];
};

struct MeshVertex {
  vec3 position;
  float pad0;
  vec3 normal;
  float pad1;
  vec3 color;
  float pad2;
};

layout(scalar, buffer_reference, buffer_reference_align = 16) buffer LbmRW {
  LbmPop cells[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) buffer MacroRW {
  vec4 values[];
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer OccupancyRW {
  uint data[];
};

layout(scalar, buffer_reference, buffer_reference_align = 16) readonly buffer MeshBuffer {
  MeshVertex vertices[];
};

const ivec3 kLbmE[19] = ivec3[](ivec3(0, 0, 0), ivec3(1, 0, 0), ivec3(-1, 0, 0), ivec3(0, 1, 0),
                                ivec3(0, -1, 0), ivec3(0, 0, 1), ivec3(0, 0, -1), ivec3(1, 1, 0),
                                ivec3(-1, 1, 0), ivec3(1, -1, 0), ivec3(-1, -1, 0), ivec3(1, 0, 1),
                                ivec3(-1, 0, 1), ivec3(1, 0, -1), ivec3(-1, 0, -1), ivec3(0, 1, 1),
                                ivec3(0, -1, 1), ivec3(0, 1, -1), ivec3(0, -1, -1));

const float kLbmW[19] =
    float[](1.0 / 3.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0,
            1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
            1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0);

const uint kLbmOpp[19] = uint[](0, 2, 1, 4, 3, 6, 5, 10, 9, 8, 7, 14, 13, 12, 11, 18, 17, 16, 15);

uint WindCellIndex(uvec3 cell, uvec3 grid) {
  return cell.x + cell.y * grid.x + cell.z * grid.x * grid.y;
}

uvec3 WindCellCoord(uint index, uvec3 grid) {
  const uint xy = grid.x * grid.y;
  const uint z = index / xy;
  const uint rem = index - z * xy;
  return uvec3(rem % grid.x, rem / grid.x, z);
}

float LbmFeq(uint k, float density, vec3 velocity) {
  const vec3 e = vec3(kLbmE[k]);
  const float eu = dot(e, velocity);
  return kLbmW[k] * density *
         (1.0 + 3.0 * eu + 4.5 * eu * eu - 1.5 * dot(velocity, velocity));
}

#endif
