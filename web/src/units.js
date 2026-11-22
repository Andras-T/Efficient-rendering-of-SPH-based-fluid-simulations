export const LatticeSpeedMin = 0.02;
export const LatticeSpeedMax = 0.14;
export const DefaultLatticeSpeed = 0.08;
export const LatticeToMetersPerSecond = 625;
export const MetersPerSecondToKmh = 3.6;
export const WindKmhMin = 50;
export const WindKmhMax = 280;
export const DefaultWindKmh =
  DefaultLatticeSpeed * LatticeToMetersPerSecond * MetersPerSecondToKmh;

export function latticeToMs(u) {
  return u * LatticeToMetersPerSecond;
}

export function latticeToKmh(u) {
  return latticeToMs(u) * MetersPerSecondToKmh;
}

export function kmhToLattice(kmh) {
  return kmh / (LatticeToMetersPerSecond * MetersPerSecondToKmh);
}

export function clampLatticeSpeed(u) {
  return Math.min(LatticeSpeedMax, Math.max(LatticeSpeedMin, u));
}

export function clampWindKmh(kmh) {
  return Math.min(WindKmhMax, Math.max(WindKmhMin, kmh));
}

export const Domain = {
  origin: [-1.7, 0, -0.64],
  cellSize: 0.04,
  grid: [96, 24, 32],
};

export function domainMax(domain = Domain) {
  return [
    domain.origin[0] + domain.cellSize * domain.grid[0],
    domain.origin[1] + domain.cellSize * domain.grid[1],
    domain.origin[2] + domain.cellSize * domain.grid[2],
  ];
}

export function cellCount(domain = Domain) {
  return domain.grid[0] * domain.grid[1] * domain.grid[2];
}
