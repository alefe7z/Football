"""Visualização e edição da malha de cabeça usada nas partidas."""

import math
import re
import sqlite3
import tkinter as tk
from tkinter import messagebox, ttk

CONTROLS = ("Largura das bochechas", "Largura da mandíbula", "Projeção do rosto", "Altura do queixo")


def read_head(path):
    for part in path.read_text(encoding="ascii", errors="replace").split("*GEOMOBJECT"):
        if re.search(r'\*NODE_NAME\s+"head"', part):
            points = {int(i): (float(x), float(y), float(z)) for i, x, y, z in
                      re.findall(r"\*MESH_VERTEX\s+(\d+)\s+(\S+)\s+(\S+)\s+(\S+)", part)}
            faces = [tuple(map(int, ids)) for ids in re.findall(
                r"\*MESH_FACE\s+\d+:\s+A:\s*(\d+)\s+B:\s*(\d+)\s+C:\s*(\d+)", part)]
            if points and faces and all(i in points for face in faces for i in face):
                return points, faces
    raise ValueError("O modelo do perfil precisa conter uma malha chamada head.")


def displacement(point, settings):
    x, y, z = point
    cheeks, jaw, depth, chin = (v / 100 for v in settings)
    # Smooth local falloffs in the shipped mesh's bind pose (metres).
    def weight(cy, cz, ry, rz):
        return math.exp(-((y - cy) / ry) ** 2 - ((z - cz) / rz) ** 2)

    cheek_weight = weight(-0.39, 1.69, 0.075, 0.042)
    jaw_weight = weight(-0.37, 1.62, 0.060, 0.040)
    front_weight = weight(-0.44, 1.67, 0.049, 0.059)
    chin_weight = weight(-0.387, 1.581, 0.045, 0.037)
    return (x * (0.35 * cheeks * cheek_weight + 0.45 * jaw * jaw_weight),
            -0.028 * depth * front_weight, -0.023 * chin * chin_weight)


def encode(points, settings):
    if not any(settings):
        return ""
    rows = ["face-v1 " + " ".join(map(str, settings))]
    for p in points.values():
        delta = displacement(p, settings)
        if any(abs(d) >= 0.000001 for d in delta):
            rows.append(" ".join(f"{v:.8f}" for v in (*p, *delta)))
    return "\n".join(rows)


def decode(data):
    if data:
        header = data.splitlines()[0].split()
        if len(header) != 5 or header[0] != "face-v1":
            raise ValueError("Formato de rosto desconhecido.")
        values = [int(v) for v in header[1:]]
        if all(-100 <= v <= 100 for v in values):
            return values
        raise ValueError("Ajustes do rosto fora do intervalo.")
    return [0] * len(CONTROLS)


def open_face(parent, model, name, skin, data, save):
    points, faces = read_head(model)
    initial = decode(data)
    window = tk.Toplevel(parent)
    window.title(f"Rosto 3D · {name}")
    window.transient(parent)
    window.grab_set()
    frame = ttk.Frame(window, padding=12)
    frame.pack()
    canvas = tk.Canvas(frame, width=320, height=300, bg="#e0e6e8", highlightthickness=0)
    canvas.grid(row=0, columnspan=2)
    sliders = [tk.IntVar(window, value=v) for v in initial]
    angle = tk.IntVar(window, value=15)
    palette = [(228, 185, 163), (183, 151, 132), (144, 115, 96), (102, 74, 55)]
    color = palette[max(1, min(4, skin)) - 1]

    def draw(*_):
        canvas.delete("all")
        coords = {}
        settings = [s.get() for s in sliders]
        a = math.radians(angle.get())
        for i, p in points.items():
            x, y, z = (v + d for v, d in zip(p, displacement(p, settings)))
            y += 0.33
            z -= 1.68
            # Undo the forward tilt of the rest pose for the portrait.
            y, z = 0.905 * y + 0.426 * z, -0.426 * y + 0.905 * z
            coords[i] = (x * math.cos(a) - y * math.sin(a),
                         x * math.sin(a) + y * math.cos(a), z)
        for face in sorted(faces, key=lambda f: sum(coords[i][1] for i in f), reverse=True):
            p, q, r = (coords[i] for i in face)
            u = [b - a for a, b in zip(p, q)]
            v = [b - a for a, b in zip(p, r)]
            nx, ny, nz = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
            length = math.sqrt(nx*nx + ny*ny + nz*nz) or 1
            light = 0.63 + 0.39 * max(0, (-0.4*nx - 0.7*ny + 0.59*nz) / length)
            shade = "#" + "".join(f"{min(255, round(c*light)):02x}" for c in color)
            canvas.create_polygon(*[(160+820*x, 150-820*z) for x, _, z in (p,q,r)],
                                  fill=shade, outline=shade)

    for row, (label, var) in enumerate(zip(("Girar", *CONTROLS), [angle, *sliders]), 1):
        ttk.Label(frame, text=label).grid(row=row, column=0, sticky="w")
        limit = 180 if var is angle else 100
        tk.Scale(frame, from_=-limit, to=limit, orient="horizontal", variable=var,
                 length=170, command=draw).grid(row=row, column=1)
    def reset():
        for var in sliders:
            var.set(0)
        draw()

    def commit():
        try:
            save(encode(points, [s.get() for s in sliders]))
        except (OSError, sqlite3.Error) as error:
            messagebox.showerror("Erro ao salvar rosto", str(error), parent=window)
            return
        window.destroy()

    ttk.Button(frame, text="Restaurar formato", command=reset).grid(row=6, column=0)
    ttk.Button(frame, text="Salvar rosto", command=commit).grid(row=6, column=1)
    draw()
    window.wait_window()
    if parent.winfo_exists():
        parent.grab_set()
