#ifndef SPH_SMOKE_GLSL
#define SPH_SMOKE_GLSL

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference2 : require

layout(scalar, buffer_reference, buffer_reference_align = 16) buffer VelocityRW {
  vec4 values[];
};

layout(scalar, buffer_reference, buffer_reference_align = 8) buffer ScalarRW {
  vec2 values[];
};

layout(scalar, buffer_reference, buffer_reference_align = 4) buffer PressureRW {
  float values[];
};

uint SmokeCellIndex(uvec3 cell, uvec3 grid) {
  return cell.x + cell.y * grid.x + cell.z * grid.x * grid.y;
}

uvec3 SmokeCellCoord(uint index, uvec3 grid) {
  const uint xy = grid.x * grid.y;
  const uint z = index / xy;
  const uint rem = index - z * xy;
  return uvec3(rem % grid.x, rem / grid.x, z);
}

bool SmokeIsWall(uvec3 cell, uvec3 grid) {
  return cell.x == 0 || cell.x + 1 == grid.x || cell.y == 0 || cell.y + 1 == grid.y ||
         cell.z == 0 || cell.z + 1 == grid.z;
}

vec3 SmokeCellCenter(uvec3 cell, vec3 origin, float cellSize) {
  return origin + (vec3(cell) + vec3(0.5)) * cellSize;
}

vec3 SmokeFetchVelocity(VelocityRW field, uvec3 cell, uvec3 grid) {
  if (any(greaterThanEqual(cell, grid)) || SmokeIsWall(cell, grid)) {
    return vec3(0.0);
  }
  return field.values[SmokeCellIndex(cell, grid)].xyz;
}

vec2 SmokeFetchScalar(ScalarRW field, uvec3 cell, uvec3 grid) {
  if (any(greaterThanEqual(cell, grid)) || SmokeIsWall(cell, grid)) {
    return vec2(0.0);
  }
  return field.values[SmokeCellIndex(cell, grid)];
}

vec3 SmokeInterpVelocity(VelocityRW field, vec3 world, vec3 origin, float cellSize, uvec3 grid) {
  const vec3 local = (world - origin) / cellSize - vec3(0.5);
  const ivec3 base = ivec3(floor(local));
  const vec3 frac = local - vec3(base);
  vec3 value = vec3(0.0);
  float weightSum = 0.0;
  for (int z = 0; z <= 1; ++z) {
    for (int y = 0; y <= 1; ++y) {
      for (int x = 0; x <= 1; ++x) {
        const ivec3 cell = base + ivec3(x, y, z);
        if (any(lessThan(cell, ivec3(0))) || any(greaterThanEqual(cell, ivec3(grid)))) {
          continue;
        }
        const float w = ((x == 1) ? frac.x : 1.0 - frac.x) * ((y == 1) ? frac.y : 1.0 - frac.y) *
                        ((z == 1) ? frac.z : 1.0 - frac.z);
        value += SmokeFetchVelocity(field, uvec3(cell), grid) * w;
        weightSum += w;
      }
    }
  }
  return weightSum > 1.0e-6 ? value / weightSum : vec3(0.0);
}

vec2 SmokeInterpScalar(ScalarRW field, vec3 world, vec3 origin, float cellSize, uvec3 grid) {
  const vec3 local = (world - origin) / cellSize - vec3(0.5);
  const ivec3 base = ivec3(floor(local));
  const vec3 frac = local - vec3(base);
  vec2 value = vec2(0.0);
  float weightSum = 0.0;
  for (int z = 0; z <= 1; ++z) {
    for (int y = 0; y <= 1; ++y) {
      for (int x = 0; x <= 1; ++x) {
        const ivec3 cell = base + ivec3(x, y, z);
        if (any(lessThan(cell, ivec3(0))) || any(greaterThanEqual(cell, ivec3(grid)))) {
          continue;
        }
        const float w = ((x == 1) ? frac.x : 1.0 - frac.x) * ((y == 1) ? frac.y : 1.0 - frac.y) *
                        ((z == 1) ? frac.z : 1.0 - frac.z);
        value += SmokeFetchScalar(field, uvec3(cell), grid) * w;
        weightSum += w;
      }
    }
  }
  return weightSum > 1.0e-6 ? value / weightSum : vec2(0.0);
}

#endif
