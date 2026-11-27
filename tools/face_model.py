"""Modelo anatômico, deformações e exportação para o motor ASE."""

import hashlib
import json
import math
import os
from pathlib import Path
import struct
import tempfile
import zlib

ASSETS = Path(__file__).with_name("face_assets")
PARTS = ("skin", "lip", "white", "iris", "pupil", "brow")
SKINS = ((228, 185, 163), (183, 151, 132), (144, 115, 96), (102, 74, 55))
IRISES = {
    "Castanha": (98, 62, 40),
    "Mel": (151, 117, 57),
    "Verde": (87, 117, 91),
    "Azul": (77, 125, 151),
}


class Head:
    def __init__(self):
        self.points = []
        self.faces = []
        for line in (ASSETS / "head.obj").read_text().splitlines():
            fields = line.split()
            if fields[0] == "v":
                self.points.append(tuple(map(float, fields[1:])))
            elif fields[0] == "usemtl":
                material = PARTS.index(fields[1])
            elif fields[0] == "f":
                self.faces.append((tuple(int(v) - 1 for v in fields[1:]), material))
        self.morphs = json.loads((ASSETS / "morphs.json").read_text())
        self.labels = [(group, label) for group, label, _, _ in self.morphs]
        self.labels.append(("Olhos", "Distância entre os olhos"))

    def deform(self, values):
        points = [list(p) for p in self.points]
        for value, (_, _, negative, positive) in zip(values, self.morphs):
            if value:
                strength = abs(value) / 100
                for i, x, y, z in positive if value > 0 else negative:
                    p = points[i]
                    p[0] += x * strength
                    p[1] += y * strength
                    p[2] += z * strength
        result = []
        for (x, y, z), (bx, by, bz) in zip(points, self.points):
            # Translate eyeball and surrounding socket together.
            influence = math.exp(
                -(((abs(bx) - 0.31) / 0.24) ** 4)
                - ((by - 7.284) / 0.24) ** 4
                - ((bz - 1.27) / 0.5) ** 4
            )
            x += math.copysign(0.072, bx) * values[-1] / 100 * influence
            # Fit the neutral scalp to the stock hairstyles, taper neck into the collar.
            result.append(
                (
                    x * 0.108,
                    (0.59 - z) * 0.107,
                    (y - 5.61) * 0.105 - max(6.2 - y, 0) * 0.23,
                )
            )
        return result


def settings(data, n):
    if not data:
        return [0] * n, "Castanha"
    header = data.splitlines()[0].split()
    if header[0] == "face-v1" and len(header) == 5:
        values = list(map(int, header[1:])) + [0] * (n - 4)
        iris = "Castanha"
    elif header[0] == "face-v2" and len(header) == n + 3:
        values = list(map(int, header[3:]))
        iris = header[2]
    else:
        raise ValueError("Formato de rosto desconhecido.")
    if iris not in IRISES or any(abs(v) > 100 for v in values):
        raise ValueError("Ajustes do rosto fora do intervalo.")
    return values, iris


def colors(skin, iris):
    base = SKINS[max(1, min(4, skin)) - 1]
    lips = tuple(round(v * k) for v, k in zip(base, (0.91, 0.72, 0.76)))
    return base, lips, (225, 218, 209), IRISES[iris], (29, 24, 23), (71, 48, 39)


def unit(vec):
    length = math.sqrt(sum(v * v for v in vec)) or 1
    return tuple(v / length for v in vec)


def normal(a, b, c):
    x, y, z = (j - i for i, j in zip(a, b))
    u, v, w = (j - i for i, j in zip(a, c))
    return y * w - z * v, z * u - x * w, x * v - y * u


def normals(points, faces):
    vertex = [[0.0, 0.0, 0.0] for _ in points]
    face = []
    for ids, _ in faces:
        n = normal(*(points[i] for i in ids))
        face.append(unit(n))
        for i in ids:
            for axis in range(3):
                vertex[i][axis] += n[axis]
    return face, list(map(unit, vertex))


def atomic_write(path, data):
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as f:
        temp = Path(f.name)
    try:
        temp.write_bytes(data)
        os.replace(temp, path)
    finally:
        temp.unlink(missing_ok=True)


def atlas(palette):
    def chunk(tag, data):
        return (
            struct.pack(">I", len(data))
            + tag
            + data
            + struct.pack(">I", zlib.crc32(tag + data))
        )

    row = b"".join(bytes(c) * 16 for c in palette)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">2I5B", len(palette) * 16, 16, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress((b"\0" + row) * 16))
        + chunk(b"IEND", b"")
    )


def ase(points, faces, texture):
    def xyz(p):
        return " ".join(f"{v:.7f}" for v in p)

    fn, vn = normals(points, faces)
    rows = [
        "*3DSMAX_ASCIIEXPORT 200",
        "*SCENE {",
        '*SCENE_FILENAME "face"',
        "}",
        "*MATERIAL_LIST {",
        "*MATERIAL_COUNT 1",
        "*MATERIAL 0 {",
        "*MATERIAL_SHINE 0.16",
        "*MATERIAL_SHINESTRENGTH 0.05",
        "*MATERIAL_SELFILLUM 0",
        "*MAP_DIFFUSE {",
        f'*BITMAP "{texture}"',
        "}",
        "}",
        "}",
        "*GEOMOBJECT {",
        '*NODE_NAME "face"',
        "*NODE_TM {",
        '*NODE_NAME "face"',
        "*INHERIT_POS 0 0 0",
        "*INHERIT_ROT 0 0 0",
        "*INHERIT_SCL 0 0 0",
        "*TM_ROW0 1 0 0",
        "*TM_ROW1 0 1 0",
        "*TM_ROW2 0 0 1",
        "}",
        "*MESH {",
        f"*MESH_NUMVERTEX {len(points)}",
        f"*MESH_NUMFACES {len(faces)}",
        "*MESH_VERTEX_LIST {",
    ]
    rows.extend(f"*MESH_VERTEX {i} {xyz(p)}" for i, p in enumerate(points))
    rows += ["}", "*MESH_FACE_LIST {"]
    rows.extend(
        f"*MESH_FACE {i}: A: {a} B: {b} C: {c}"
        for i, ((a, b, c), _) in enumerate(faces)
    )
    rows += ["}", f"*MESH_NUMTVERTEX {3*len(PARTS)}", "*MESH_TVERTLIST {"]
    for m in range(len(PARTS)):
        for j, (u, v) in enumerate(((0.3, 0.3), (0.7, 0.3), (0.5, 0.7))):
            rows.append(f"*MESH_TVERT {3*m+j} {(m+u)/len(PARTS)} {v} 0")
    rows += ["}", f"*MESH_NUMTVFACES {len(faces)}", "*MESH_TFACELIST {"]
    rows.extend(
        f"*MESH_TFACE {i} {3*m} {3*m+1} {3*m+2}" for i, (_, m) in enumerate(faces)
    )
    rows += ["}", "*MESH_NORMALS {"]
    for i, (ids, _) in enumerate(faces):
        rows.append(f"*MESH_FACENORMAL {i} {xyz(fn[i])}")
        rows.extend(f"*MESH_VERTEXNORMAL {j} {xyz(vn[j])}" for j in ids)
    return ("\n".join(rows + ["}", "}", "*MATERIAL_REF 0", "}"]) + "\n").encode()


DIRECTORY = Path("media/objects/players/faces")


def eyelid_mesh(eye_spacing, texture):
    # Spherical patches rotate about the common eye axis in neck-local space.
    points, faces = [], []
    for side in (-1, 1):
        start = len(points)
        for column in range(13):
            azimuth = math.pi * (column + 0.01) / 12.02
            for row in range(11):
                elevation = -0.62 + row * 0.18
                radius = 0.0175 * math.sin(azimuth)
                points.append(
                    (
                        side * eye_spacing + 0.0168 * math.cos(azimuth),
                        -radius * math.cos(elevation),
                        radius * math.sin(elevation),
                    )
                )
        for column in range(12):
            for row in range(10):
                i = start + column * 11 + row
                faces.extend([((i, i + 1, i + 11), 0), ((i + 1, i + 12, i + 11), 0)])
    return ase(points, faces, texture)


def export(profile, head, values, skin, iris):
    folder = profile / DIRECTORY
    folder.mkdir(parents=True, exist_ok=True)
    png = atlas(colors(skin, iris))
    name = "face-palette-" + hashlib.sha256(png).hexdigest()[:20] + ".png"
    atomic_write(folder / name, png)
    data = ase(head.deform(values), head.faces, (DIRECTORY / name).as_posix())
    digest = hashlib.sha256(data).hexdigest()
    atomic_write(folder / (digest + ".ase"), data)
    spacing = (0.308 + 0.072 * values[-1] / 100) * 0.108
    atomic_write(
        folder / (digest + ".lids.ase"),
        eyelid_mesh(spacing, (DIRECTORY / name).as_posix()),
    )
    return " ".join(["face-v2", digest, iris, *map(str, values)])
