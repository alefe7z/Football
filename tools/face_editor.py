"""Editor visual dos traços do rosto, integrado ao elenco."""

import math
import sqlite3
import tkinter as tk
from tkinter import messagebox, ttk

from face_model import Head, IRISES, colors, export, normals, settings


def open_face(parent, profile, name, skin, data, save):
    head = Head()
    initial, initial_iris = settings(data, len(head.labels))
    window = tk.Toplevel(parent)
    window.title(f"Rosto 3D · {name}")
    window.transient(parent)
    window.grab_set()
    panel = ttk.Frame(window, padding=12)
    panel.pack()
    canvas = tk.Canvas(panel, width=356, height=390, bg="#222933", highlightthickness=0)
    canvas.grid(row=0, column=0, rowspan=2, padx=(0, 12))
    variables = [tk.IntVar(window, value=v) for v in initial]
    iris = tk.StringVar(window, value=initial_iris)
    yaw = tk.DoubleVar(window, value=10)
    tabs = ttk.Notebook(panel)
    tabs.grid(row=0, column=1, sticky="new")
    groups = {}
    pending = None

    def draw():
        nonlocal pending
        pending = None
        pts = head.deform([v.get() for v in variables])
        c, s = math.cos(math.radians(yaw.get())), math.sin(math.radians(yaw.get()))
        rot = [(c * x - s * y, s * x + c * y, z) for x, y, z in pts]
        projected = [(178 + 1150 * x, 372 - 1150 * z) for x, y, z in rot]
        faces, vertex_normals = normals(rot, head.faces)
        palette = colors(skin, iris.get())
        # Interpolate light across each triangle and depth-test overlapping
        # features so eyelids, irises and lips remain visible at close range.
        width, height = 356, 390
        pixels = bytearray(bytes((34, 41, 51)) * (width * height))
        depths = [float("inf")] * (width * height)
        light = [
            min(255, round(145 + 108 * max(0, -0.48 * x - 0.68 * y + 0.52 * z)))
            for x, y, z in vertex_normals
        ]
        shades = [
            [
                bytes(min(255, int(channel * k / 230)) for channel in rgb)
                for k in range(256)
            ]
            for rgb in palette
        ]
        for i, (ids, material) in enumerate(head.faces):
            if faces[i][1] >= 0:
                continue
            corners = sorted(
                (projected[j][1], projected[j][0], rot[j][1], light[j]) for j in ids
            )
            lo, mid, hi = corners
            for top, bottom in ((lo, mid), (mid, hi)):
                if bottom[0] <= top[0] or hi[0] <= lo[0]:
                    continue
                for y in range(
                    max(0, math.ceil(top[0] - 0.5)),
                    min(height, math.ceil(bottom[0] - 0.5)),
                ):
                    t = (y + 0.5 - top[0]) / (bottom[0] - top[0])
                    u = (y + 0.5 - lo[0]) / (hi[0] - lo[0])
                    a = [top[k] + t * (bottom[k] - top[k]) for k in (1, 2, 3)]
                    b = [lo[k] + u * (hi[k] - lo[k]) for k in (1, 2, 3)]
                    if a[0] > b[0]:
                        a, b = b, a
                    first, last = max(0, math.ceil(a[0] - 0.5)), min(
                        width, math.ceil(b[0] - 0.5)
                    )
                    if first >= last:
                        continue
                    dz = (b[1] - a[1]) / (b[0] - a[0])
                    dl = (b[2] - a[2]) / (b[0] - a[0])
                    depth = a[1] + (first + 0.5 - a[0]) * dz
                    value = a[2] + (first + 0.5 - a[0]) * dl
                    for index in range(y * width + first, y * width + last):
                        if depth < depths[index]:
                            depths[index] = depth
                            pixels[index * 3 : index * 3 + 3] = shades[material][
                                round(value)
                            ]
                        depth += dz
                        value += dl
        frame = f"P6\n{width} {height}\n255\n".encode() + pixels
        canvas.portrait = tk.PhotoImage(data=frame, format="PPM")
        canvas.delete("face")
        canvas.create_image(0, 0, anchor="nw", image=canvas.portrait, tags="face")

    def redraw(*_):
        nonlocal pending
        if pending is None:
            pending = window.after_idle(draw)

    for var, (group, label) in zip(variables, head.labels):
        if group not in groups:
            groups[group] = ttk.Frame(tabs, padding=10)
            tabs.add(groups[group], text=group)
        ttk.Label(groups[group], text=label).pack(anchor="w")
        tk.Scale(
            groups[group],
            variable=var,
            from_=-100,
            to=100,
            orient="horizontal",
            length=235,
            command=redraw,
        ).pack(fill="x")
    ttk.Label(groups["Olhos"], text="Cor da íris").pack(anchor="w")
    picker = ttk.Combobox(
        groups["Olhos"], state="readonly", values=list(IRISES), textvariable=iris
    )
    picker.pack(fill="x")
    picker.bind("<<ComboboxSelected>>", redraw)
    drag_start = [0, 0.0]

    def start_rotation(event):
        drag_start[:] = event.x, yaw.get()

    def rotate(event):
        yaw.set((drag_start[1] + 0.7 * (event.x - drag_start[0]) + 180) % 360 - 180)
        redraw()

    canvas.bind("<ButtonPress-1>", start_rotation)
    canvas.bind("<B1-Motion>", rotate)
    turn = ttk.Frame(panel)
    turn.grid(row=2, column=0, sticky="ew", padx=(0, 12))
    ttk.Label(turn, text="Girar").pack(side="left")
    tk.Scale(
        turn,
        from_=-180,
        to=180,
        orient="horizontal",
        variable=yaw,
        command=redraw,
        showvalue=False,
    ).pack(side="left", fill="x", expand=True)

    def reset():
        for v in variables:
            v.set(0)
        iris.set("Castanha")
        redraw()

    def commit():
        try:
            save(export(profile, head, [v.get() for v in variables], skin, iris.get()))
        except (OSError, sqlite3.Error, ValueError) as error:
            messagebox.showerror("Erro ao salvar rosto", str(error), parent=window)
            return
        window.destroy()

    actions = ttk.Frame(panel)
    actions.grid(row=2, column=1, sticky="e")
    ttk.Button(actions, text="Restaurar traços", command=reset).pack(side="left")
    ttk.Button(actions, text="Salvar rosto", command=commit).pack(side="left", padx=6)
    draw()
    window.wait_window()
    if parent.winfo_exists():
        parent.grab_set()
