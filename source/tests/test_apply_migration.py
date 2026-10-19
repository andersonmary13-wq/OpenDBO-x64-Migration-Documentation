"""I exercise the real applier using disposable synthetic Git repositories only."""
import contextlib
import difflib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

MODULE = Path(__file__).resolve().parents[1] / "tools/apply_migration.py"
spec = importlib.util.spec_from_file_location("migration_applier", MODULE)
app = importlib.util.module_from_spec(spec)
spec.loader.exec_module(app)


def pin(raw):
    return {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}


def fmt(raw):
    encoding = "raw-bytes"
    text = raw
    if raw.startswith(b"\xff\xfe"):
        encoding = "utf16le-bom"
        text = raw[2:].decode("utf-16le").encode()
    elif raw.startswith(b"\xef\xbb\xbf"):
        encoding = "utf8-bom"
        text = raw[3:]
    positions = [i for i, line in enumerate(text.split(b"\n")[:-1]) if line.endswith(b"\r")]
    count = text.count(b"\n")
    mode = "LF" if not positions else ("CRLF" if len(positions) == count else "mixed")
    return {"encoding": encoding, "newline_mode": mode, "newline_count": count,
            "crlf_line_indices": positions if mode == "mixed" else [], "final_newline": text.endswith(b"\n")}


def zero_patch(name, before, after):
    old = app.project(before).decode() if before is not None else ""
    new = app.project(after).decode()
    lines = difflib.unified_diff(old.splitlines(keepends=True), new.splitlines(keepends=True),
                                 fromfile="a/" + name if before is not None else "/dev/null",
                                 tofile="b/" + name, n=0)
    result = ["diff --git a/%s b/%s\n" % (name, name)]
    if before is None:
        result.append("new file mode 100644\n")
    for line in lines:
        result.append(line if line.endswith("\n") else line + "\n\\ No newline at end of file\n")
    return "".join(result).encode()


class ApplyTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="opendbo-applier-test-")
        self.root = Path(self.tmp.name)
        self.core = self.root / "core"
        self.bundle = self.root / "bundle"
        self.core.mkdir()
        self.bundle.mkdir()
        self.initial = {
            "A.cpp": b"\xff\xfe" + "one\r\nold\r\n".encode("utf-16le"),
            "D.cpp": b"first\r\nold\nlast\r\nend",
            "B.h": b"value old\n",
        }
        for name, raw in self.initial.items():
            (self.core / name).write_bytes(raw)
        self.git("init", "--quiet")
        self.git("add", "--all")
        self.git("-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid",
                 "commit", "--quiet", "-m", "Synthetic base")
        self.head = self.git("rev-parse", "HEAD").strip().decode()
        self.git("checkout", "--quiet", "-b", "x64-migration")
        self.settings = patch.multiple(app, EXPECTED_BASE=self.head,
                                      EXPECTED_STAGES=(("baseline191", 2), ("migration35", 3)),
                                      EXPECTED_NEW_SOURCES=1, EXPECTED_UNION_COUNT=4)
        self.settings.start()
        self.s1 = {"A.cpp": b"\xff\xfe" + "one\r\nbase\r\n".encode("utf-16le"),
                   "D.cpp": b"first\r\nbase\nlast\r\nend"}
        self.s2 = {"A.cpp": b"\xff\xfe" + "one\r\nfinal\r\n".encode("utf-16le"),
                   "B.h": b"value final\r\n", "C.h": b"\xef\xbb\xbfnew\n"}
        self.manifest = {"schema": app.SCHEMA, "base_commit": self.head, "stages": [],
                         "new_files": [], "final_union_count": 4, "final_union_files": []}
        previous = dict(self.initial)
        for sid, changes in [("baseline191", self.s1), ("migration35", self.s2)]:
            rows = []
            patch_data = b""
            for name, after in changes.items():
                before = previous.get(name)
                raw_before = b"value old\r\n" if name == "B.h" else before
                row = {"path": name, "before": pin(raw_before) if before is not None else None,
                       "after": pin(after), "before_projection": pin(app.project(before)) if before is not None else None,
                       "after_projection": pin(app.project(after)), "after_format": fmt(after),
                       "accepted_before_raw": [pin(raw_before)] if before is not None else []}
                if raw_before is not None:
                    row["before_format"] = fmt(raw_before)
                if name == "B.h":
                    row["accepted_before_raw"].append(pin(before))
                if before is None:
                    row["new_source"] = "new-files/" + name
                    target = self.bundle / row["new_source"]
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(after)
                    self.manifest["new_files"].append({"path": name, "package_path": row["new_source"], **pin(after)})
                patch_data += zero_patch(name, before, after)
                rows.append(row)
                previous[name] = after
            patch_name = sid + ".patch"
            (self.bundle / patch_name).write_bytes(patch_data)
            self.manifest["stages"].append({"id": sid, "files": rows,
                                             "patch": {"path": patch_name, **pin(patch_data)}})
        self.final = previous
        self.manifest["final_union_files"] = [{"path": k, **pin(v)} for k, v in previous.items()]
        self.save()

    def tearDown(self):
        self.settings.stop()
        self.tmp.cleanup()

    def git(self, *args):
        result = subprocess.run(["git", "-c", "core.autocrlf=false", *args], cwd=self.core,
                                env=app.git_env(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertEqual(result.returncode, 0, "Synthetic Git command failed")
        return result.stdout

    def save(self):
        self.path = self.bundle / "source-export-manifest.json"
        self.path.write_text(json.dumps(self.manifest), encoding="utf-8")
        self.sha = app.digest(self.path.read_bytes())

    def run_main(self, *args):
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            return app.main([str(self.core), "--manifest", str(self.path), *args])

    def assert_initial(self):
        for name, raw in self.initial.items():
            self.assertEqual((self.core / name).read_bytes(), raw)
        self.assertFalse((self.core / "C.h").exists())

    def test_check_is_default_and_does_not_touch_checkout_or_index(self):
        index = (self.core / ".git/index").read_bytes()
        self.assertEqual(self.run_main(), 0)
        self.assert_initial()
        self.assertEqual((self.core / ".git/index").read_bytes(), index)
        self.assertEqual(self.git("rev-parse", "HEAD").strip().decode(), self.head)

    def test_apply_exact_raw_all_stages_without_staging(self):
        original_mkdtemp = tempfile.mkdtemp
        def located(*args, **kwargs):
            if kwargs.get("prefix") == "opendbo-migration-backup-":
                kwargs["dir"] = self.root
            return original_mkdtemp(*args, **kwargs)
        with patch.object(app.tempfile, "mkdtemp", side_effect=located):
            self.assertEqual(self.run_main("--apply", "--branch", "x64-migration", "--manifest-sha256", self.sha), 0)
        for name, raw in self.final.items():
            self.assertEqual((self.core / name).read_bytes(), raw)
        self.assertEqual(self.git("diff", "--cached", "--name-only"), b"")

    def test_main_branch_refused(self):
        self.git("branch", "-m", "main")
        self.assertEqual(self.run_main("--apply", "--branch", "main", "--manifest-sha256", self.sha), 2)
        self.assert_initial()

    def test_apply_requires_explicit_branch_and_manifest_pin(self):
        self.assertEqual(self.run_main("--apply"), 2)
        self.assert_initial()

    def test_wrong_head_refused(self):
        with patch.object(app, "EXPECTED_BASE", "0" * 40):
            self.assertEqual(self.run_main(), 2)
        self.assert_initial()

    def test_staged_change_refused(self):
        (self.core / "B.h").write_bytes(b"changed\n")
        self.git("add", "B.h")
        self.assertEqual(self.run_main(), 2)

    def test_untracked_change_refused(self):
        (self.core / "unexpected.txt").write_bytes(b"unknown")
        self.assertEqual(self.run_main(), 2)
        self.assert_initial()

    def test_unaccepted_raw_revision_refused(self):
        (self.core / "B.h").write_bytes(b"wrong\n")
        with self.assertRaises(app.Refusal):
            app.check_series(self.core, self.manifest, self.bundle)

    def test_explicit_upstream_lf_alternate_and_mixed_bom_roundtrip(self):
        m, _ = app.load_manifest(self.path, self.sha)
        _, final = app.check_series(self.core, m, self.bundle)
        self.assertEqual(final, self.final)
        for raw in [b"a\rb\nend", b"a\r\nb\nend", b"\xef\xbb\xbfx\r\n"]:
            self.assertEqual(app.restore(app.project(raw), fmt(raw)), raw)

    def test_checkout_eol_fallback_is_exactly_reconstructible(self):
        expected = b"first\nsecond\nend"
        row = {"path": "sample.cpp", "before": pin(expected), "before_projection": pin(expected),
               "before_format": fmt(expected), "accepted_before_raw": [pin(expected)]}
        app.before_ok(b"first\r\nsecond\r\nend", row)
        app.before_ok(b"first\r\nsecond\nend", row)
        with self.assertRaises(app.Refusal):
            app.before_ok(b"first\r\nsecond\r\nend", row, allow_checkout_eol=False)
        for wrong in [b"\xef\xbb\xbffirst\r\nsecond\r\nend", b"first\r\r\nsecond\r\nend",
                      b"first\r\nCHANGED\r\nend", b"first\r\nsecond\r\nend\r\n"]:
            with self.assertRaises(app.Refusal):
                app.before_ok(wrong, row)
        row["before"]["sha256"] = "0" * 64
        with self.assertRaises(app.Refusal):
            app.before_ok(b"first\r\nsecond\r\nend", row)

    def test_bad_patch_hash_refused(self):
        (self.bundle / "baseline191.patch").write_bytes(b"corrupt")
        self.assertEqual(self.run_main(), 2)
        self.assert_initial()

    def test_bad_new_source_refused(self):
        (self.bundle / "new-files/C.h").write_bytes(b"corrupt")
        self.assertEqual(self.run_main(), 2)
        self.assert_initial()

    def test_outside_patch_membership_refused(self):
        stage = self.manifest["stages"][0]
        file = self.bundle / stage["patch"]["path"]
        raw = file.read_bytes() + zero_patch("unselected.h", None, b"no\n")
        file.write_bytes(raw)
        stage["patch"].update(pin(raw))
        self.save()
        self.assertEqual(self.run_main(), 2)
        self.assert_initial()

    def test_traversal_and_git_metadata_refused(self):
        for name in ["../escape", "/absolute", ".git/config", "a/../../b", "a\\b", "x:y"]:
            with self.assertRaises(app.Refusal):
                app.relative(name)

    def test_bad_format_and_union_refused(self):
        self.manifest["stages"][0]["files"][0]["after_format"]["newline_count"] += 1
        self.save()
        self.assertEqual(self.run_main(), 2)
        self.assert_initial()
        self.manifest["final_union_count"] = 999
        self.save()
        self.assertEqual(self.run_main(), 2)

    def test_baseline_only_keeps_independent_win32_stage(self):
        self.assertEqual(self.run_main("--check", "--stage", "baseline"), 0)
        self.assert_initial()

    def test_second_stage_failure_rolls_back_first_stage(self):
        original_stage = app.run_stage
        original_mkdtemp = tempfile.mkdtemp
        actual_stages = []
        def located(*args, **kwargs):
            if kwargs.get("prefix") == "opendbo-migration-backup-":
                kwargs["dir"] = self.root
            return original_mkdtemp(*args, **kwargs)
        def failing(work, bundle, stage, checkout_eol_paths=None):
            if work == self.core:
                actual_stages.append(stage["id"])
            if work == self.core and stage["id"] == "migration35":
                raise app.Refusal("Synthetic second-stage failure")
            return original_stage(work, bundle, stage, checkout_eol_paths)
        with patch.object(app, "run_stage", side_effect=failing), patch.object(app.tempfile, "mkdtemp", side_effect=located):
            self.assertEqual(self.run_main("--apply", "--branch", "x64-migration", "--manifest-sha256", self.sha), 2)
        self.assert_initial()
        self.assertEqual(actual_stages, ["baseline191", "migration35"])
        self.assertEqual(self.git("status", "--porcelain"), b"")


if __name__ == "__main__":
    unittest.main()
