#!/usr/bin/env python3
"""Cluster the formula-car STL into a closed triangle mesh for the WebGPU app."""

from __future__ import annotations

import math
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets" / "formula-car" / "c42.stl"
DEST = ROOT / "web" / "public" / "c42.bin"
CELL = 0.06


def load_stl(path: Path) -> list[tuple[tuple[float, float, float], ...]]:
    data = path.read_bytes()
    count = struct.unpack_from("<I", data, 80)[0]
    offset = 84
    tris: list[tuple[tuple[float, float, float], ...]] = []
    for _ in range(count):
        verts = tuple(struct.unpack_from("<fff", data, offset + 12 + k * 12) for k in range(3))
        tris.append(verts)
        offset += 50
    return tris


def cluster(tris: list[tuple[tuple[float, float, float], ...]], cell: float):
    buckets: dict[tuple[int, int, int], list[float]] = {}
    faces: list[tuple[tuple[int, int, int], tuple[int, int, int], tuple[int, int, int]]] = []
    for a, b, c in tris:
        keys = []
        for p in (a, b, c):
            key = (round(p[0] / cell), round(p[1] / cell), round(p[2] / cell))
            acc = buckets.get(key)
            if acc is None:
                buckets[key] = [p[0], p[1], p[2], 1.0]
            else:
                acc[0] += p[0]
                acc[1] += p[1]
                acc[2] += p[2]
                acc[3] += 1.0
            keys.append(key)
        if len(set(keys)) == 3:
            faces.append((keys[0], keys[1], keys[2]))
    keys = list(buckets.keys())
    index = {key: i for i, key in enumerate(keys)}
    positions = []
    for key in keys:
        x, y, z, n = buckets[key]
        positions.append((x / n, y / n, z / n))
    indices = [(index[a], index[b], index[c]) for a, b, c in faces]
    return positions, indices


def smooth_normals(positions, indices):
    normals = [[0.0, 0.0, 0.0] for _ in positions]
    for ia, ib, ic in indices:
        ax, ay, az = positions[ia]
        bx, by, bz = positions[ib]
        cx, cy, cz = positions[ic]
        ux, uy, uz = bx - ax, by - ay, bz - az
        vx, vy, vz = cx - ax, cy - ay, cz - az
        nx = uy * vz - uz * vy
        ny = uz * vx - ux * vz
        nz = ux * vy - uy * vx
        for i in (ia, ib, ic):
            normals[i][0] += nx
            normals[i][1] += ny
            normals[i][2] += nz
    out = []
    for nx, ny, nz in normals:
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        if length < 1e-12:
            out.append((0.0, 1.0, 0.0))
        else:
            out.append((nx / length, ny / length, nz / length))
    return out


def write_packed(path: Path, positions, normals, indices) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as fh:
        fh.write(b"C42M")
        fh.write(struct.pack("<II", len(positions), len(indices)))
        for x, y, z in positions:
            fh.write(struct.pack("<fff", x, y, z))
        for x, y, z in normals:
            fh.write(struct.pack("<fff", x, y, z))
        for a, b, c in indices:
            fh.write(struct.pack("<III", a, b, c))


def main() -> None:
    tris = load_stl(SOURCE)
    positions, indices = cluster(tris, CELL)
    normals = smooth_normals(positions, indices)
    write_packed(DEST, positions, normals, indices)
    print(f"wrote {DEST} verts={len(positions)} tris={len(indices)} bytes={DEST.stat().st_size}")


if __name__ == "__main__":
    main()
