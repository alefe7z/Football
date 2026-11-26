"""Editor gráfico de conteúdo do Gameplay Football (Python 3 + Tk)."""

import argparse
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import tempfile
import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk
from contextlib import closing
from uuid import uuid4

from face_editor import open_face


SOURCE = Path(__file__).resolve().parents[1] / "data"
DATABASE = Path("databases/default/database.sqlite")
TEXTURES = {
    "Bola": "media/objects/balls/ball.jpg",
    "Chuteira": "media/objects/players/textures/shoe.jpg",
    "Solado": "media/objects/players/textures/shoe_sole.jpg",
}


def prepare_profile(source, profile):
    marker = profile / ".football-editor-v1"
    if marker.is_file() and (profile / DATABASE).is_file():
        return
    if profile.exists() or source in profile.parents:
        raise ValueError("Escolha uma pasta nova para o perfil.")
    profile.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=profile.parent) as tmp:
        stage = Path(tmp) / "profile"
        stage.mkdir()
        for folder in ("databases", "media", "locale"):
            shutil.copytree(source / folder, stage / folder)
        shutil.copyfile(source / "football.config", stage / "football.config")
        for category, rel in TEXTURES.items():
            library = stage / "catalogo" / category
            library.mkdir(parents=True)
            shutil.copyfile(stage / rel, library / "Original.jpg")
        (stage / marker.name).touch()
        stage.rename(profile)


def replace_file(source, target):
    with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as f:
        temp = Path(f.name)
    try:
        shutil.copyfile(source, temp)
        os.replace(temp, target)
    finally:
        if temp.exists():
            temp.unlink()


def clone_team(profile, template, league, name, short):
    root = profile / DATABASE.parent
    if not name.strip() or not short.strip():
        raise ValueError("Informe o nome e a sigla.")
    with closing(sqlite3.connect(root / DATABASE.name)) as db, db:
        db.row_factory = sqlite3.Row
        db.execute("BEGIN IMMEDIATE")
        if db.execute("SELECT 1 FROM teams WHERE league_id=? AND name=? COLLATE NOCASE",
                      (league, name.strip())).fetchone():
            raise ValueError("Já existe um time com esse nome nesta liga.")
        club = dict(db.execute("SELECT * FROM teams WHERE id=?", (template,)).fetchone())
        players = db.execute("SELECT * FROM players WHERE team_id=? ORDER BY formationorder",
                             (template,)).fetchall()
        if len(players) < 11:
            raise ValueError("Escolha um modelo com pelo menos 11 jogadores.")
        relative = Path("images_teams") / "editor" / uuid4().hex
        folder = root / relative
        folder.mkdir(parents=True)
        try:
            shutil.copyfile(root / club["logo_url"], folder / "logo.png")
            for suffix in ("_kit_01.png", "_kit_02.png"):
                shutil.copyfile(root / (club["kit_url"] + suffix), folder / ("club" + suffix))
            club.update(league_id=league, name=name.strip(), shortname=short.strip(),
                        logo_url=(relative / "logo.png").as_posix(),
                        kit_url=(relative / "club").as_posix())
            club_id = insert_copy(db, "teams", club)
            for row in players:
                player = dict(row)
                player.update(team_id=club_id, nationalteam_id=-1, nationalteamformationorder=-1)
                insert_copy(db, "players", player)
            db.commit()
        except Exception:
            shutil.rmtree(folder)
            raise
        return club_id


def insert_copy(db, table, row):
    del row["id"]
    columns = ", ".join(row)
    placeholders = ", ".join("?" for _ in row)
    return db.execute(f"INSERT INTO {table} ({columns}) VALUES ({placeholders})",
                      list(row.values())).lastrowid


class Editor:
    def __init__(self, window, profile):
        self.window = window
        self.profile = profile
        self.process = None
        window.title("Editor de conteúdo · Football")
        frame = ttk.Frame(window, padding=16)
        frame.pack(fill="both", expand=True)
        ttk.Label(frame, text="Clubes e elencos").pack(anchor="w")
        self.league = self.selector(frame, "Liga de destino")
        self.club = self.selector(frame, "Clube para editar ou usar como modelo")
        self.refresh_clubs()
        for label, action in (("Criar clube a partir do modelo", self.create_club),
                              ("Editar jogador e rosto 3D", self.edit_player),
                              ("Trocar escudo", lambda: self.club_image("logo_url", "")),
                              ("Trocar uniforme titular", lambda: self.club_image("kit_url", "_kit_01.png")),
                              ("Trocar uniforme reserva", lambda: self.club_image("kit_url", "_kit_02.png"))):
            self.button(frame, label, action)
        ttk.Separator(frame).pack(fill="x", pady=8)
        self.category = self.selector(frame, "Equipamento")
        self.category["values"] = tuple(TEXTURES)
        self.category.current(0)
        self.item = self.selector(frame, "Itens disponíveis")
        self.category.bind("<<ComboboxSelected>>", lambda _: self.refresh_items())
        self.refresh_items()
        self.button(frame, "Importar textura JPEG", self.import_item)
        self.button(frame, "Usar item selecionado", self.activate)
        self.button(frame, "Abrir pasta do perfil", self.open_folder)
        self.button(frame, "Jogar com este perfil", self.launch)
        self.status = tk.StringVar(value=str(profile))
        ttk.Label(frame, textvariable=self.status, wraplength=450).pack(pady=(8, 0))

    def selector(self, frame, label):
        ttk.Label(frame, text=label).pack(anchor="w")
        combo = ttk.Combobox(frame, state="readonly", width=48)
        combo.pack(fill="x", pady=(0, 5))
        return combo

    def button(self, frame, label, action):
        def run():
            try:
                if self.process and self.process.poll() is None and action != self.open_folder:
                    raise ValueError("Feche o jogo antes de continuar.")
                action()
            except (OSError, sqlite3.Error, ValueError) as error:
                messagebox.showerror("Editor de conteúdo", str(error))
        ttk.Button(frame, text=label, command=run).pack(fill="x")

    def query(self, sql, params=()):
        with closing(sqlite3.connect(self.profile / DATABASE)) as db, db:
            return db.execute(sql, params).fetchall()

    def refresh_clubs(self, selected=None):
        self.clubs = self.query("SELECT id, name FROM teams ORDER BY name, id")
        self.leagues = self.query("SELECT id, name FROM leagues ORDER BY name, id")
        for combo, rows in ((self.club, self.clubs), (self.league, self.leagues)):
            combo["values"] = [f"{name} ({key})" for key, name in rows]
            if rows and combo.current() < 0:
                combo.current(0)
        if selected:
            self.club.current(next(i for i, row in enumerate(self.clubs) if row[0] == selected))

    def create_club(self):
        name = simpledialog.askstring("Novo clube", "Nome do clube")
        if not name:
            return
        short = simpledialog.askstring("Novo clube", "Sigla para o placar")
        if short:
            key = clone_team(self.profile, self.clubs[self.club.current()][0],
                             self.leagues[self.league.current()][0], name, short)
            self.refresh_clubs(key)
            self.status.set(f"{name} foi criado com elenco e uniformes próprios.")

    def edit_player(self):
        club = self.clubs[self.club.current()][0]
        players = self.query("SELECT id, firstname, lastname FROM players WHERE team_id=? "
                             "ORDER BY formationorder", (club,))
        dialog = tk.Toplevel(self.window)
        dialog.title("Elenco")
        dialog.transient(self.window)
        dialog.grab_set()
        combo = self.selector(dialog, "Jogador")
        combo["values"] = [f"{a} {b}" for _, a, b in players]
        combo.current(0)

        def rename():
            key, first, last = players[combo.current()]
            a = simpledialog.askstring("Jogador", "Nome", initialvalue=first, parent=dialog)
            if a is None:
                return
            b = simpledialog.askstring("Jogador", "Sobrenome", initialvalue=last, parent=dialog)
            if b and b.strip():
                self.query("UPDATE players SET firstname=?, lastname=? WHERE id=?",
                           (a.strip(), b.strip(), key))
                dialog.destroy()
        def face():
            key, first, last = players[combo.current()]
            columns = {row[1] for row in self.query("PRAGMA table_info(players)")}
            field = "face_shape" if "face_shape" in columns else "''"
            skin, data = self.query(f"SELECT skincolor, {field} FROM players WHERE id=?", (key,))[0]

            def save(value):
                with closing(sqlite3.connect(self.profile / DATABASE)) as db, db:
                    db.execute("BEGIN IMMEDIATE")
                    if "face_shape" not in {r[1] for r in db.execute("PRAGMA table_info(players)")}:
                        db.execute("ALTER TABLE players ADD COLUMN face_shape TEXT DEFAULT ''")
                    db.execute("UPDATE players SET face_shape=? WHERE id=?", (value, key))
                self.status.set(f"Rosto de {first} {last} salvo.")

            open_face(dialog, self.profile / "media/objects/players/models/fullbody.ase",
                      f"{first} {last}", int(skin or 1), data, save)

        self.button(dialog, "Editar nome", rename)
        self.button(dialog, "Editar rosto 3D", face)

    def club_image(self, field, suffix):
        club = self.clubs[self.club.current()][0]
        rel, = self.query(f"SELECT {field} FROM teams WHERE id=?", (club,))[0]
        file = filedialog.askopenfilename(filetypes=[("Imagem PNG", "*.png")])
        if file:
            with open(file, "rb") as f:
                if f.read(8) != b"\x89PNG\r\n\x1a\n":
                    raise ValueError("Selecione uma imagem PNG.")
            replace_file(file, self.profile / DATABASE.parent / (rel + suffix))
            self.status.set("Imagem do clube atualizada.")

    def library(self):
        return self.profile / "catalogo" / self.category.get()

    def refresh_items(self, name="Original.jpg"):
        items = sorted(p.name for p in self.library().glob("*.jpg"))
        self.item["values"] = items
        self.item.set(name if name in items else "")

    def import_item(self):
        file = filedialog.askopenfilename(filetypes=[("Textura JPEG", "*.jpg *.jpeg")])
        if file:
            path = Path(file)
            with path.open("rb") as f:
                if f.read(3) != b"\xff\xd8\xff":
                    raise ValueError("Selecione uma textura JPEG.")
            target = self.library() / (path.stem + ".jpg")
            if target.exists():
                raise ValueError("Já existe um item com esse nome. Renomeie o arquivo.")
            replace_file(path, target)
            self.refresh_items(target.name)
            self.status.set("Textura importada. Selecione Usar item para aplicá-la.")

    def activate(self):
        replace_file(self.library() / self.item.get(), self.profile / TEXTURES[self.category.get()])
        self.status.set(f"{self.category.get()} em uso: {self.item.get()}")

    def open_folder(self):
        import sys
        if sys.platform == "win32":
            os.startfile(self.profile)
        else:
            subprocess.Popen(["open" if sys.platform == "darwin" else "xdg-open", str(self.profile)])

    def launch(self):
        file = filedialog.askopenfilename(title="Selecione o executável gameplayfootball")
        if file:
            self.process = subprocess.Popen([file], env={**os.environ, "FOOTBALL_CONTENT_DIR": str(self.profile)})
            self.status.set("Jogo iniciado com este perfil.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--perfil", type=Path, default=SOURCE.parent / "profiles" / "meu-futebol")
    args = parser.parse_args()
    window = tk.Tk()
    try:
        profile = args.perfil.resolve()
        prepare_profile(SOURCE, profile)
        Editor(window, profile)
    except (ValueError, OSError, sqlite3.Error) as error:
        messagebox.showerror("Editor de conteúdo", str(error))
        window.destroy()
    else:
        window.mainloop()
