import sqlite3
import sys
import tempfile
import unittest
from contextlib import closing
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from content_editor import DATABASE, SOURCE, TEXTURES, clone_team, prepare_profile, replace_file


class ContentEditorTest(unittest.TestCase):
    def test_profile_and_playable_clubs(self):
        with tempfile.TemporaryDirectory() as tmp:
            profile = Path(tmp) / "liga"
            prepare_profile(SOURCE, profile)
            with closing(sqlite3.connect(profile / DATABASE)) as db:
                original = db.execute("SELECT count(*) FROM teams").fetchone()[0]
                for i in range(100):
                    key = clone_team(profile, 1, 2, f"Clube d'Água {i}", "CDA")
                self.assertEqual(db.execute("SELECT count(*) FROM teams").fetchone()[0], original + 100)
                roster = "SELECT firstname, lastname, formationorder FROM players WHERE team_id=? ORDER BY formationorder"
                self.assertEqual(db.execute(roster, (key,)).fetchall(), db.execute(roster, (1,)).fetchall())
                club = db.execute("SELECT logo_url, kit_url FROM teams WHERE id=? AND league_id=2", (key,)).fetchone()
                for image in (club[0], club[1] + "_kit_01.png", club[1] + "_kit_02.png"):
                    self.assertTrue((profile / DATABASE.parent / image).is_file())
                with self.assertRaises(ValueError):
                    clone_team(profile, 1, 2, "Clube d'Água 99", "DUP")
                self.assertEqual(db.execute("PRAGMA integrity_check").fetchone(), ("ok",))

            ball = profile / TEXTURES["Bola"]
            source_ball = SOURCE / TEXTURES["Bola"]
            replace_file(profile / TEXTURES["Chuteira"], ball)
            prepare_profile(SOURCE, profile)
            self.assertNotEqual(ball.read_bytes(), source_ball.read_bytes())
            replace_file(profile / "catalogo/Bola/Original.jpg", ball)
            self.assertEqual(ball.read_bytes(), source_ball.read_bytes())


if __name__ == "__main__":
    unittest.main()
