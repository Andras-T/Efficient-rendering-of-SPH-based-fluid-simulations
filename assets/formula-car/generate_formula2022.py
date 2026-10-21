#!/usr/bin/env python3
"""Build an original FIA-style 2022 open-wheeler as a binary STL."""

from __future__ import annotations

import math
import struct
from pathlib import Path

Pi = math.pi


class Vec:
    __slots__ = ("x", "y", "z")

    def __init__(self, x: float, y: float, z: float = 0.0) -> None:
        self.x = float(x)
        self.y = float(y)
        self.z = float(z)

    def __add__(self, other: Vec) -> Vec:
        return Vec(self.x + other.x, self.y + other.y, self.z + other.z)

    def __sub__(self, other: Vec) -> Vec:
        return Vec(self.x - other.x, self.y - other.y, self.z - other.z)

    def __mul__(self, s: float) -> Vec:
        return Vec(self.x * s, self.y * s, self.z * s)

    __rmul__ = __mul__

    def __neg__(self) -> Vec:
        return Vec(-self.x, -self.y, -self.z)

    def dot(self, other: Vec) -> float:
        return self.x * other.x + self.y * other.y + self.z * other.z

    def cross(self, other: Vec) -> Vec:
        return Vec(
            self.y * other.z - self.z * other.y,
            self.z * other.x - self.x * other.z,
            self.x * other.y - self.y * other.x,
        )

    def length(self) -> float:
        return math.sqrt(self.dot(self))

    def normalized(self) -> Vec:
        length = self.length()
        return self if length < 1.0e-12 else self * (1.0 / length)


def lerp(a: Vec, b: Vec, t: float) -> Vec:
    return a * (1.0 - t) + b * t


class Mesh:
    def __init__(self) -> None:
        self.tris: list[tuple[Vec, Vec, Vec]] = []

    def add_tri(self, a: Vec, b: Vec, c: Vec) -> None:
        if (b - a).cross(c - a).length() < 1.0e-12:
            return
        self.tris.append((a, b, c))

    def add_quad(self, a: Vec, b: Vec, c: Vec, d: Vec) -> None:
        self.add_tri(a, b, c)
        self.add_tri(a, c, d)

    def add_grid(self, points: list[list[Vec]], flip: bool = False) -> None:
        for v in range(len(points) - 1):
            row0 = points[v]
            row1 = points[v + 1]
            for u in range(min(len(row0), len(row1)) - 1):
                a, b = row0[u], row0[u + 1]
                d, c = row1[u], row1[u + 1]
                if flip:
                    self.add_quad(a, d, c, b)
                else:
                    self.add_quad(a, b, c, d)

    def add_cap(self, ring: list[Vec], center: Vec, flip: bool = False) -> None:
        for i in range(len(ring) - 1):
            if flip:
                self.add_tri(center, ring[i + 1], ring[i])
            else:
                self.add_tri(center, ring[i], ring[i + 1])

    def write_stl(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        header = b"FIA-style 2022 open-wheeler (CC0)" + b"\0" * 80
        with path.open("wb") as handle:
            handle.write(header[:80])
            handle.write(struct.pack("<I", len(self.tris)))
            for a, b, c in self.tris:
                normal = (b - a).cross(c - a).normalized()
                handle.write(
                    struct.pack(
                        "<12fH",
                        normal.x,
                        normal.y,
                        normal.z,
                        a.x,
                        a.y,
                        a.z,
                        b.x,
                        b.y,
                        b.z,
                        c.x,
                        c.y,
                        c.z,
                        0,
                    )
                )


def naca_camber(x: float, m: float, p: float) -> tuple[float, float]:
    if p <= 1.0e-6:
        return 0.0, 0.0
    if x < p:
        yc = m / (p * p) * (2.0 * p * x - x * x)
        dyc = 2.0 * m / (p * p) * (p - x)
    else:
        den = (1.0 - p) * (1.0 - p)
        yc = m / den * ((1.0 - 2.0 * p) + 2.0 * p * x - x * x)
        dyc = 2.0 * m / den * (p - x)
    return yc, dyc


def naca4_loop(code: str, samples: int, invert: bool) -> list[tuple[float, float]]:
    m = int(code[0]) / 100.0
    p = int(code[1]) / 10.0
    thick = int(code[2:]) / 100.0
    loop: list[tuple[float, float]] = []
    # Trailing edge along upper surface to the leading edge, then lower back.
    for side, sign in (("upper", 1.0), ("lower", -1.0)):
        count = samples if side == "upper" else samples - 1
        for i in range(count):
            if side == "upper":
                t = i / (samples - 1)
            else:
                t = 1.0 - (i + 1) / (samples - 1)
            x = 0.5 * (1.0 - math.cos(Pi * t))
            yt = (
                5.0
                * thick
                * (
                    0.2969 * math.sqrt(max(x, 0.0))
                    - 0.1260 * x
                    - 0.3516 * x * x
                    + 0.2843 * x**3
                    - 0.1015 * x**4
                )
            )
            yc, dyc = naca_camber(x, m, p)
            theta = math.atan(dyc)
            px = x - sign * yt * math.sin(theta)
            py = yc + sign * yt * math.cos(theta)
            if invert:
                py = -py
            loop.append((px, py))
    return loop


def transform_airfoil(
    loop: list[tuple[float, float]],
    origin: Vec,
    chord: float,
    incidence: float,
    z: float,
) -> list[Vec]:
    ca = math.cos(incidence)
    sa = math.sin(incidence)
    out: list[Vec] = []
    for x, y in loop:
        lx = (x - 1.0) * chord
        ly = y * chord
        rx = lx * ca - ly * sa
        ry = lx * sa + ly * ca
        out.append(Vec(origin.x + rx, origin.y + ry, z))
    return out


def add_lofted_solid(mesh: Mesh, stations: list[list[Vec]]) -> None:
    mesh.add_grid(stations, flip=False)
    mesh.add_grid(stations, flip=True)
    mesh.add_cap(stations[0], centroid(stations[0]), flip=True)
    mesh.add_cap(stations[-1], centroid(stations[-1]), flip=False)


def centroid(points: list[Vec]) -> Vec:
    acc = Vec(0.0, 0.0, 0.0)
    for point in points:
        acc = acc + point
    return acc * (1.0 / float(len(points)))


def add_wing(
    mesh: Mesh,
    origin: Vec,
    half_span: float,
    chord: float,
    incidence: float,
    naca: str,
    span_samples: int,
    chord_samples: int,
    sweep: float = 0.0,
    taper: float = 1.0,
    anhedral: float = 0.0,
    invert: bool = True,
) -> None:
    loop = naca4_loop(naca, chord_samples, invert)
    stations: list[list[Vec]] = []
    for i in range(span_samples):
        t = i / (span_samples - 1)
        z = -half_span + 2.0 * half_span * t
        local_chord = chord * (1.0 - (1.0 - taper) * abs(z) / max(half_span, 1.0e-4))
        station_origin = Vec(origin.x + sweep * abs(z), origin.y + anhedral * abs(z), 0.0)
        stations.append(transform_airfoil(loop, station_origin, local_chord, incidence, z))
    add_lofted_solid(mesh, stations)


def add_plate(mesh: Mesh, corners: list[Vec], thickness: float) -> None:
    # corners are a closed planar quad (a,b,c,d) on the outer face.
    a, b, c, d = corners
    normal = (b - a).cross(d - a).normalized()
    offset = normal * thickness
    inner = [p - offset for p in (a, b, c, d)]
    mesh.add_quad(a, b, c, d)
    mesh.add_quad(inner[0], inner[3], inner[2], inner[1])
    mesh.add_quad(a, d, inner[3], inner[0])
    mesh.add_quad(b, a, inner[0], inner[1])
    mesh.add_quad(c, b, inner[1], inner[2])
    mesh.add_quad(d, c, inner[2], inner[3])


def add_box(mesh: Mesh, mn: Vec, mx: Vec) -> None:
    p = [
        Vec(mn.x, mn.y, mn.z),
        Vec(mx.x, mn.y, mn.z),
        Vec(mx.x, mx.y, mn.z),
        Vec(mn.x, mx.y, mn.z),
        Vec(mn.x, mn.y, mx.z),
        Vec(mx.x, mn.y, mx.z),
        Vec(mx.x, mx.y, mx.z),
        Vec(mn.x, mx.y, mx.z),
    ]
    faces = ((1, 5, 6, 2), (0, 3, 7, 4), (3, 2, 6, 7), (0, 4, 5, 1), (4, 7, 6, 5), (1, 2, 3, 0))
    for face in faces:
        mesh.add_quad(p[face[0]], p[face[1]], p[face[2]], p[face[3]])


def ellipse_point(rx: float, ry: float, angle: float) -> tuple[float, float]:
    return rx * math.cos(angle), ry * math.sin(angle)


def add_revolve(mesh: Mesh, profile: list[Vec], axis_y: float, samples: int) -> None:
    rings: list[list[Vec]] = []
    for i in range(samples + 1):
        ang = 2.0 * Pi * i / samples
        ca, sa = math.cos(ang), math.sin(ang)
        ring: list[Vec] = []
        for point in profile:
            # profile in (x = axial, y = radius) at z=0, revolved around x through (0, axis_y, 0)
            # Here profile.x is axial (world x), profile.y is radius in yz.
            r = point.y
            ring.append(Vec(point.x, axis_y + r * ca, r * sa))
        rings.append(ring)
    mesh.add_grid(rings)


def add_wheel(mesh: Mesh, center: Vec, radius: float, half_width: float, samples: int = 28) -> None:
    # Tire profile in (axial x, radius y); revolve around wheel axis (z).
    # Build as a torus-like solid by revolving a rounded rectangle around z.
    profile: list[Vec] = []
    # Cross-section in the xy plane relative to wheel center, then rotate about z.
    # Use parametric: theta around tire tube.
    tube_pts = 12
    tube_r = min(half_width * 0.92, radius * 0.28)
    major = radius - tube_r
    for i in range(tube_pts + 1):
        a = Pi * 1.15 + (2.0 * Pi - 0.4 * Pi) * i / tube_pts  # open inner slightly for rim
        profile.append(Vec(math.cos(a) * tube_r, major + math.sin(a) * tube_r, 0.0))
    rings: list[list[Vec]] = []
    for i in range(samples + 1):
        ang = 2.0 * Pi * i / samples
        ca, sa = math.cos(ang), math.sin(ang)
        ring = []
        for p in profile:
            # p.x is along wheel axis (z), p.y is radial in xy of wheel
            radial_x = p.y * ca
            radial_y = p.y * sa
            ring.append(Vec(center.x + radial_x, center.y + radial_y, center.z + p.x))
        rings.append(ring)
    mesh.add_grid(rings)
    # Rim disc (outer and inner faces) plus barrel.
    rim_r = major * 0.72
    barrel: list[list[Vec]] = []
    for i in range(samples + 1):
        ang = 2.0 * Pi * i / samples
        ca, sa = math.cos(ang), math.sin(ang)
        outer = Vec(center.x + rim_r * ca, center.y + rim_r * sa, center.z + half_width * 0.55)
        inner = Vec(center.x + rim_r * ca, center.y + rim_r * sa, center.z - half_width * 0.55)
        hub_o = Vec(center.x + 0.06 * ca, center.y + 0.06 * sa, center.z + half_width * 0.20)
        hub_i = Vec(center.x + 0.06 * ca, center.y + 0.06 * sa, center.z - half_width * 0.20)
        barrel.append([outer, hub_o, hub_i, inner])
    mesh.add_grid(barrel)
    # 2022-style wheel cover on the outer face.
    cover: list[list[Vec]] = []
    z_cover = center.z + (half_width + 0.012 if center.z > 0.0 else -(half_width + 0.012))
    z_in = center.z + (half_width * 0.70 if center.z > 0.0 else -half_width * 0.70)
    for i in range(samples + 1):
        ang = 2.0 * Pi * i / samples
        ca, sa = math.cos(ang), math.sin(ang)
        cover.append(
            [
                Vec(center.x + 0.04 * ca, center.y + 0.04 * sa, z_cover),
                Vec(center.x + radius * 0.93 * ca, center.y + radius * 0.93 * sa, z_cover),
                Vec(center.x + radius * 0.93 * ca, center.y + radius * 0.93 * sa, z_in),
            ]
        )
    mesh.add_grid(cover)


def add_halo(mesh: Mesh, center: Vec, radius: float, tube: float, samples: int = 28) -> None:
    rings: list[list[Vec]] = []
    tube_n = 8
    for i in range(samples + 1):
        # Three-quarter loop plus frontal stay is built as a full ring then a stay bar.
        ang = -0.55 * Pi + 1.55 * Pi * i / samples
        ca, sa = math.cos(ang), math.sin(ang)
        ring_c = Vec(center.x + radius * sa * 0.35, center.y + radius * ca, 0.0)
        ring: list[Vec] = []
        for k in range(tube_n + 1):
            ta = 2.0 * Pi * k / tube_n
            # local frame: along loop
            tangent = Vec(radius * 0.35 * ca, -radius * sa, 0.0).normalized()
            bitangent = Vec(0.0, 0.0, 1.0)
            normal = tangent.cross(bitangent).normalized()
            ring.append(ring_c + (normal * math.cos(ta) + bitangent * math.sin(ta)) * tube)
        rings.append(ring)
    mesh.add_grid(rings)
    # Frontal stay.
    stay_a = Vec(center.x - radius * 0.05, center.y + radius * 0.15, 0.0)
    stay_b = Vec(center.x - radius * 0.85, center.y + 0.04, 0.0)
    add_box(
        mesh,
        Vec(min(stay_a.x, stay_b.x) - 0.012, stay_b.y - 0.012, -0.012),
        Vec(max(stay_a.x, stay_b.x) + 0.012, stay_a.y + tube, 0.012),
    )


def add_nose(mesh: Mesh, samples: int = 20) -> None:
    # Loft shrinking ellipses from cockpit to tip. +x is rear.
    stations: list[list[Vec]] = []
    for i in range(samples):
        t = i / (samples - 1)
        x = -2.35 + 1.55 * t
        rx = 0.05 + 0.22 * (t**1.35)
        ry = 0.04 + 0.16 * (t**1.15)
        y = 0.19 + 0.04 * t
        ring: list[Vec] = []
        n = 20
        for k in range(n + 1):
            a = 2.0 * Pi * k / n
            px, py = ellipse_point(rx, ry, a)
            ring.append(Vec(x, y + py, px))
        stations.append(ring)
    mesh.add_grid(stations)
    mesh.add_cap(stations[0], centroid(stations[0]), flip=True)


def add_chassis(mesh: Mesh) -> None:
    stations: list[list[Vec]] = []
    sections = [
        # x, half_width, height, y_floor
        (-0.80, 0.28, 0.28, 0.12),
        (-0.10, 0.34, 0.38, 0.10),
        (0.55, 0.32, 0.36, 0.10),
        (1.20, 0.22, 0.30, 0.12),
        (1.85, 0.16, 0.22, 0.16),
    ]
    n = 18
    for x, hw, h, y0 in sections:
        ring: list[Vec] = []
        for k in range(n + 1):
            a = 2.0 * Pi * k / n
            # Slightly squared cockpit section.
            c = abs(math.cos(a)) ** 0.65 * math.copysign(1.0, math.cos(a))
            s = abs(math.sin(a)) ** 0.75 * math.copysign(1.0, math.sin(a))
            ring.append(Vec(x, y0 + 0.5 * h + 0.5 * h * s, hw * c))
        stations.append(ring)
    mesh.add_grid(stations)
    mesh.add_cap(stations[-1], centroid(stations[-1]), flip=False)
    # Cockpit opening lip.
    add_box(mesh, Vec(-0.18, 0.42, -0.18), Vec(0.42, 0.46, 0.18))
    add_halo(mesh, Vec(0.08, 0.46, 0.0), 0.28, 0.018)


def add_sidepod(mesh: Mesh, sign: float) -> None:
    stations: list[list[Vec]] = []
    # 2022 undercut: inlet high, body tucks in at the floor.
    specs = [
        (0.05, 0.42, 0.10, 0.22, 0.16),
        (0.45, 0.58, 0.08, 0.26, 0.22),
        (0.95, 0.62, 0.09, 0.24, 0.20),
        (1.45, 0.48, 0.12, 0.18, 0.14),
        (1.90, 0.28, 0.16, 0.12, 0.10),
    ]
    n = 14
    for x, z_out, y0, h, half_h in specs:
        z_in = sign * 0.22
        z_mid = sign * z_out
        ring: list[Vec] = []
        for k in range(n + 1):
            t = k / n
            a = Pi * t  # 0..pi around the outer half
            y = y0 + 0.5 * h + 0.5 * h * math.cos(a)
            z = 0.5 * (z_in + z_mid) + 0.5 * (z_mid - z_in) * math.sin(a)
            # Undercut: pull lower points inward.
            if y < y0 + 0.35 * h:
                z = z * 0.72 + z_in * 0.28
            ring.append(Vec(x, y, z))
        # Close against the tub.
        ring.append(Vec(x, y0, z_in))
        ring.append(Vec(x, y0 + h, z_in * 0.85))
        stations.append(ring)
    mesh.add_grid(stations)
    # Inlet lip.
    z0 = sign * 0.34
    z1 = sign * 0.58
    add_box(mesh, Vec(-0.02, 0.22, min(z0, z1)), Vec(0.10, 0.38, max(z0, z1)))


def add_floor(mesh: Mesh) -> None:
    xs = 36
    zs = 22
    x0, x1 = -0.95, 2.15
    z_half = 0.72
    top: list[list[Vec]] = []
    bot: list[list[Vec]] = []
    for i in range(xs):
        tx = i / (xs - 1)
        x = x0 + (x1 - x0) * tx
        row_t: list[Vec] = []
        row_b: list[Vec] = []
        for j in range(zs):
            tz = j / (zs - 1)
            z = -z_half + 2.0 * z_half * tz
            # Keel down the center, venturi channels outboard.
            channel = 0.055 * math.exp(-((abs(z) - 0.38) ** 2) / 0.045)
            if x > 1.35:
                ramp = (x - 1.35) / 0.80
                channel += 0.16 * ramp * ramp
            y_top = 0.055 + (0.10 if x > 1.45 else 0.0) * max(x - 1.45, 0.0) / 0.70
            if x > 1.45:
                y_top = 0.055 + 0.22 * ((x - 1.45) / 0.70) ** 1.15
            y_bot = 0.012 + channel
            # Floor edge fence.
            if abs(z) > 0.66:
                y_top += 0.035
            row_t.append(Vec(x, y_top, z))
            row_b.append(Vec(x, y_bot, z))
        top.append(row_t)
        bot.append(row_b)
    mesh.add_grid(top)
    mesh.add_grid(bot, flip=True)
    mesh.add_grid([[top[i][0] for i in range(xs)], [bot[i][0] for i in range(xs)]])
    mesh.add_grid([[bot[i][-1] for i in range(xs)], [top[i][-1] for i in range(xs)]])
    mesh.add_quad(top[0][0], top[0][-1], bot[0][-1], bot[0][0])
    mesh.add_quad(top[-1][-1], top[-1][0], bot[-1][0], bot[-1][-1])
    # Diffuser strakes.
    for z in (-0.52, -0.28, 0.28, 0.52):
        add_plate(
            mesh,
            [
                Vec(1.40, 0.02, z),
                Vec(2.12, 0.02, z),
                Vec(2.12, 0.24, z),
                Vec(1.55, 0.08, z),
            ],
            0.010,
        )
    # Floor fences / bargeboard remnants ahead of the sidepods.
    for sign in (-1.0, 1.0):
        z = sign * 0.62
        add_plate(
            mesh,
            [
                Vec(-0.55, 0.02, z),
                Vec(0.15, 0.02, z),
                Vec(0.15, 0.16, z),
                Vec(-0.50, 0.12, z),
            ],
            0.008,
        )
        add_plate(
            mesh,
            [
                Vec(-0.48, 0.04, sign * 0.48),
                Vec(-0.10, 0.04, sign * 0.55),
                Vec(-0.10, 0.18, sign * 0.55),
                Vec(-0.48, 0.16, sign * 0.48),
            ],
            0.007,
        )


def add_airbox(mesh: Mesh) -> None:
    stations: list[list[Vec]] = []
    n = 14
    for i, (x, rx, ry, y) in enumerate(
        ((0.10, 0.07, 0.05, 0.62), (0.42, 0.09, 0.11, 0.70), (0.85, 0.06, 0.08, 0.58), (1.25, 0.04, 0.04, 0.42))
    ):
        ring = []
        for k in range(n + 1):
            a = 2.0 * Pi * k / n
            px, py = ellipse_point(rx, ry, a)
            ring.append(Vec(x, y + py, px))
        stations.append(ring)
    mesh.add_grid(stations)
    mesh.add_cap(stations[0], centroid(stations[0]), flip=True)
    mesh.add_cap(stations[-1], centroid(stations[-1]), flip=False)


def add_gearbox(mesh: Mesh) -> None:
    stations: list[list[Vec]] = []
    n = 12
    for x, hw, h, y0 in ((1.70, 0.14, 0.18, 0.16), (2.05, 0.10, 0.14, 0.18), (2.28, 0.07, 0.10, 0.22)):
        ring = []
        for k in range(n + 1):
            a = 2.0 * Pi * k / n
            ring.append(Vec(x, y0 + 0.5 * h + 0.5 * h * math.sin(a), hw * math.cos(a)))
        stations.append(ring)
    mesh.add_grid(stations)
    mesh.add_cap(stations[-1], centroid(stations[-1]), flip=False)


def add_front_wing(mesh: Mesh) -> None:
    # 2022-style four-element full-width wing, close to the ground.
    add_wing(
        mesh,
        origin=Vec(-2.15, 0.075, 0.0),
        half_span=0.98,
        chord=0.38,
        incidence=-0.10,
        naca="4412",
        span_samples=36,
        chord_samples=21,
        sweep=0.04,
        taper=0.86,
        invert=True,
    )
    add_wing(
        mesh,
        origin=Vec(-1.82, 0.105, 0.0),
        half_span=0.94,
        chord=0.22,
        incidence=-0.22,
        naca="4410",
        span_samples=32,
        chord_samples=17,
        sweep=0.05,
        taper=0.88,
        invert=True,
    )
    add_wing(
        mesh,
        origin=Vec(-1.64, 0.128, 0.0),
        half_span=0.90,
        chord=0.16,
        incidence=-0.32,
        naca="3410",
        span_samples=30,
        chord_samples=15,
        sweep=0.05,
        taper=0.90,
        invert=True,
    )
    add_wing(
        mesh,
        origin=Vec(-1.52, 0.148, 0.0),
        half_span=0.86,
        chord=0.12,
        incidence=-0.40,
        naca="2408",
        span_samples=28,
        chord_samples=13,
        sweep=0.04,
        taper=0.92,
        invert=True,
    )
    for sign in (-1.0, 1.0):
        z = sign * 0.99
        add_plate(
            mesh,
            [
                Vec(-2.22, 0.01, z),
                Vec(-1.42, 0.01, z),
                Vec(-1.42, 0.22, z),
                Vec(-2.18, 0.20, z),
            ],
            0.012,
        )
        add_plate(
            mesh,
            [
                Vec(-2.20, 0.01, z - sign * 0.10),
                Vec(-1.70, 0.01, z - sign * 0.02),
                Vec(-1.70, 0.01, z + sign * 0.01),
                Vec(-2.20, 0.01, z + sign * 0.01),
            ],
            0.012,
        )


def add_rear_wing(mesh: Mesh) -> None:
    add_wing(
        mesh,
        origin=Vec(2.42, 0.78, 0.0),
        half_span=0.48,
        chord=0.32,
        incidence=-0.18,
        naca="4412",
        span_samples=24,
        chord_samples=19,
        sweep=0.08,
        taper=0.92,
        invert=True,
    )
    add_wing(
        mesh,
        origin=Vec(2.58, 0.86, 0.0),
        half_span=0.46,
        chord=0.20,
        incidence=-0.38,
        naca="3410",
        span_samples=22,
        chord_samples=15,
        sweep=0.06,
        taper=0.95,
        invert=True,
    )
    # Beam wing (2022).
    add_wing(
        mesh,
        origin=Vec(2.22, 0.38, 0.0),
        half_span=0.42,
        chord=0.22,
        incidence=-0.12,
        naca="3410",
        span_samples=20,
        chord_samples=15,
        taper=0.90,
        invert=True,
    )
    for sign in (-1.0, 1.0):
        z = sign * 0.49
        add_plate(
            mesh,
            [
                Vec(2.12, 0.30, z),
                Vec(2.68, 0.30, z),
                Vec(2.68, 0.98, z),
                Vec(2.18, 0.92, z),
            ],
            0.010,
        )


def add_mirrors_and_vanes(mesh: Mesh) -> None:
    for sign in (-1.0, 1.0):
        add_box(mesh, Vec(-0.05, 0.40, sign * 0.38 - 0.03), Vec(0.08, 0.48, sign * 0.38 + 0.03))
        add_box(mesh, Vec(-0.02, 0.44, sign * 0.20), Vec(0.02, 0.47, sign * 0.38))
        # Turning vanes ahead of the sidepod.
        add_plate(
            mesh,
            [
                Vec(-0.35, 0.06, sign * 0.36),
                Vec(0.05, 0.06, sign * 0.42),
                Vec(0.05, 0.20, sign * 0.42),
                Vec(-0.35, 0.18, sign * 0.36),
            ],
            0.006,
        )


def build() -> Mesh:
    mesh = Mesh()
    add_front_wing(mesh)
    add_nose(mesh)
    add_chassis(mesh)
    add_sidepod(mesh, 1.0)
    add_sidepod(mesh, -1.0)
    add_floor(mesh)
    add_airbox(mesh)
    add_gearbox(mesh)
    add_rear_wing(mesh)
    add_mirrors_and_vanes(mesh)
    # 18-inch wheels, 2022 proportions.
    add_wheel(mesh, Vec(-1.42, 0.36, 0.82), 0.360, 0.155)
    add_wheel(mesh, Vec(-1.42, 0.36, -0.82), 0.360, 0.155)
    add_wheel(mesh, Vec(1.95, 0.36, 0.80), 0.360, 0.205)
    add_wheel(mesh, Vec(1.95, 0.36, -0.80), 0.360, 0.205)
    return mesh


def main() -> None:
    out = Path(__file__).with_name("formula2022.stl")
    mesh = build()
    mesh.write_stl(out)
    xs = [p.x for tri in mesh.tris for p in tri]
    ys = [p.y for tri in mesh.tris for p in tri]
    zs = [p.z for tri in mesh.tris for p in tri]
    print(
        f"wrote {out}  tris={len(mesh.tris)}  "
        f"x=[{min(xs):.2f},{max(xs):.2f}] y=[{min(ys):.2f},{max(ys):.2f}] "
        f"z=[{min(zs):.2f},{max(zs):.2f}]"
    )


if __name__ == "__main__":
    main()
