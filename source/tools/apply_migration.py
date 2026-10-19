#!/usr/bin/env python3
"""I check or apply the two reviewed source stages; I do not build or run a client."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import sys
import tempfile

EXPECTED_BASE = "ed30bbe393653c704130aa026baf2bcad02c31ff"
EXPECTED_STAGES = (("baseline191", 191), ("migration35", 35))
EXPECTED_NEW_SOURCES = 9
EXPECTED_UNION_COUNT = 225
SCHEMA = "opendbo-source-patch-series/v1"


class Refusal(Exception):
    pass


def require(condition, message):
    if not condition:
        raise Refusal(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def relative(value):
    require(isinstance(value, str) and value and "\\" not in value,
            "I require a nonempty portable relative path.")
    p = PurePosixPath(value)
    require(not p.is_absolute() and all(x not in ("", ".", "..") for x in value.split("/")),
            "I refuse an absolute or traversing path.")
    require(all(ord(c) >= 32 and c not in ':<>"|?*' for c in value),
            "I refuse a nonportable path.")
    require(all(x.casefold() != ".git" for x in p.parts), "I refuse a Git metadata target.")
    return p


def no_links(root, rel):
    path = root
    for part in (None, *relative(rel).parts):
        if part is not None:
            path = path / part
        require(not path.is_symlink() and not getattr(path, "is_junction", lambda: False)(),
                "I refuse a symlink or junction in a selected path.")
    return path


def check_pin(data, pin, label):
    require(isinstance(pin, dict) and type(pin.get("bytes")) is int and pin["bytes"] >= 0,
            "I require an explicit byte count: " + label)
    require(re.fullmatch(r"[0-9a-f]{64}", str(pin.get("sha256", ""))) is not None,
            "I require a SHA-256 pin: " + label)
    require(len(data) == pin["bytes"] and digest(data) == pin["sha256"],
            "I found a byte/hash mismatch: " + label)


def project(raw):
    if raw.startswith(b"\xff\xfe"):
        data = raw[2:].decode("utf-16le", errors="strict").encode("utf-8")
    elif raw.startswith(b"\xef\xbb\xbf"):
        data = raw[3:]
    else:
        data = raw
    data = data.replace(b"\r\n", b"\n")
    require(b"\0" not in data, "I refuse a binary/NUL projection.")
    return data


def restore(data, fmt):
    require(b"\0" not in data and b"\r\n" not in data, "I require an LF-only text projection.")
    require(isinstance(fmt, dict), "I require an explicit format recipe.")
    count = data.count(b"\n")
    require(type(fmt.get("newline_count")) is int and fmt["newline_count"] == count,
            "I found a newline-count mismatch.")
    require(type(fmt.get("final_newline")) is bool and fmt["final_newline"] == data.endswith(b"\n"),
            "I found a final-newline mismatch.")
    mode = fmt.get("newline_mode")
    indices = fmt.get("crlf_line_indices")
    require(isinstance(indices, list) and all(type(x) is int for x in indices),
            "I require explicit CRLF indices.")
    require(indices == sorted(set(indices)) and all(0 <= x < count for x in indices),
            "I refuse invalid CRLF indices.")
    require(mode in ("LF", "CRLF", "mixed"), "I refuse an unknown newline mode.")
    require(mode == "mixed" or not indices, "I refuse indices in a uniform newline recipe.")
    use_crlf = set(range(count)) if mode == "CRLF" else set(indices)
    parts = data.split(b"\n")
    data = b"".join(part + ((b"\r\n" if i in use_crlf else b"\n") if i < count else b"")
                    for i, part in enumerate(parts))
    encoding = fmt.get("encoding")
    if encoding == "utf16le-bom":
        return b"\xff\xfe" + data.decode("utf-8", errors="strict").encode("utf-16le")
    if encoding == "utf8-bom":
        return b"\xef\xbb\xbf" + data
    require(encoding == "raw-bytes", "I refuse an unknown encoding recipe.")
    return data


def git_env():
    env = {k: v for k, v in os.environ.items() if not k.startswith("GIT_")}
    env.update(GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull,
               GIT_OPTIONAL_LOCKS="0", GIT_TERMINAL_PROMPT="0")
    return env


def git(cwd, *args, allow_failure=False, extra=()):
    command = ["git", "-c", "core.fsmonitor=false", "-c", "core.hooksPath=",
               "-c", "core.autocrlf=false", "-c", "core.safecrlf=false", *extra, *args]
    result = subprocess.run(command, cwd=cwd, env=git_env(), stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, check=False)
    require(allow_failure or result.returncode == 0,
            "I could not complete a local Git verification (exit %d)." % result.returncode)
    return result


def disabled_filters(core):
    result = git(core, "config", "--name-only", "--get-regexp", r"^filter\..*\.(clean|smudge|process|required)$",
                 allow_failure=True)
    require(result.returncode in (0, 1), "I could not inspect local filter names.")
    extra = []
    for name in result.stdout.decode("utf-8", errors="strict").splitlines():
        require(re.fullmatch(r"filter\.[^\r\n=]+\.(?:clean|smudge|process|required)", name),
                "I refuse an invalid Git filter name.")
        extra.extend(("-c", name + ("=false" if name.endswith(".required") else "=")))
    return extra


def checkout_state(core, expected_branch=None, allowed_dirty=()):
    require(core.is_dir() and not core.is_symlink() and not getattr(core, "is_junction", lambda: False)(),
            "I require a regular explicit Core checkout root.")
    top = Path(git(core, "rev-parse", "--show-toplevel").stdout.decode().strip()).resolve()
    require(top == core, "I require the exact Git worktree root, not a subdirectory.")
    head = git(core, "rev-parse", "HEAD").stdout.decode().strip()
    require(head == EXPECTED_BASE, "I require the tested upstream base commit.")
    branch_result = git(core, "symbolic-ref", "--quiet", "--short", "HEAD", allow_failure=True)
    branch = branch_result.stdout.decode().strip() if branch_result.returncode == 0 else None
    if expected_branch is not None:
        require(branch == expected_branch, "I require the explicitly named working branch.")
        require(re.fullmatch(r"(?:[A-Za-z0-9._-]+/)*x64(?:[-/][A-Za-z0-9._/-]+)?", branch or "") is not None,
                "I apply only on a dedicated x64 working branch, never main/master/release/stable.")
    status = git(core, "status", "--porcelain=v1", "-z", "--untracked-files=all",
                 "--ignore-submodules=none", extra=disabled_filters(core)).stdout
    dirty = set()
    for row in status.split(b"\0"):
        if not row:
            continue
        require(len(row) >= 4 and row[2:3] == b" " and b"R" not in row[:2] and b"C" not in row[:2],
                "I refuse renamed or copied worktree entries.")
        require(row[:1] in (b" ", b"?"), "I refuse staged changes.")
        name = row[3:].decode("utf-8", errors="strict")
        relative(name)
        dirty.add(name)
    require(dirty <= set(allowed_dirty), "I require a clean checkout outside my selected paths.")
    return {"head": head, "branch": branch, "dirty": sorted(dirty)}


def load_manifest(path, expected_sha=None):
    raw = path.read_bytes()
    if expected_sha is not None:
        require(digest(raw) == expected_sha, "I found a manifest hash mismatch.")
    data = json.loads(raw)
    require(data.get("schema") == SCHEMA and data.get("base_commit") == EXPECTED_BASE,
            "I refuse an unknown manifest or base commit.")
    stages = data.get("stages")
    require(isinstance(stages, list) and len(stages) == 2, "I require exactly two source stages.")
    new_count = 0
    all_paths = set()
    for stage, (sid, count) in zip(stages, EXPECTED_STAGES):
        require(stage.get("id") == sid and len(stage.get("files", [])) == count,
                "I found a stage identity or membership-count mismatch.")
        paths = [str(relative(row["path"])) for row in stage["files"]]
        require(len(paths) == len(set(x.casefold() for x in paths)), "I refuse duplicate/case-colliding targets.")
        all_paths.update(paths)
        patch = stage.get("patch", {})
        relative(patch.get("path"))
        patch_path = no_links(path.parent, patch["path"])
        check_pin(patch_path.read_bytes(), patch, sid + " patch")
        for row in stage["files"]:
            require(row.get("after") is not None and row.get("after_projection") is not None,
                    "I do not support source deletion in this series.")
            if row.get("new_source") is not None:
                rel = relative(row["new_source"])
                require(rel.parts[0] == "new-files", "I require a bounded new-source audit path.")
                raw_new = no_links(path.parent, str(rel)).read_bytes()
                check_pin(raw_new, row["after"], row["path"] + " new source")
                check_pin(project(raw_new), row["after_projection"], row["path"] + " new projection")
                new_count += 1
    require(new_count == EXPECTED_NEW_SOURCES, "I found an unexpected new-source audit count.")
    require(len(all_paths) == len({x.casefold() for x in all_paths}), "I refuse union case collisions.")
    declared = data.get("final_union_files", [])
    require(data.get("final_union_count") == EXPECTED_UNION_COUNT and
            len(declared) == EXPECTED_UNION_COUNT and
            {str(relative(r["path"])) for r in declared} == all_paths,
            "I found an unexpected final union membership.")
    final = {r["path"]: r["after"] for stage in stages for r in stage["files"]}
    require(all({"bytes": r["bytes"], "sha256": r["sha256"]} == final[r["path"]] for r in declared),
            "I found a conflicting final union pin.")
    require(len(data.get("new_files", [])) == EXPECTED_NEW_SOURCES,
            "I require the full new-source audit list.")
    for row in data["new_files"]:
        path_new = str(relative(row["path"]))
        require(path_new in final and {"bytes": row["bytes"], "sha256": row["sha256"]} == final[path_new],
                "I found a conflicting new-source pin.")
        package = str(relative(row["package_path"]))
        require(package == "new-files/" + path_new, "I refuse an unexpected new-source location.")
        check_pin(no_links(path.parent, package).read_bytes(), row, path_new + " packaged source")
    return data, raw


def patch_membership(work, patch):
    result = git(work, "apply", "--no-index", "--unidiff-zero", "--numstat", "-z", str(patch))
    names = []
    for entry in result.stdout.split(b"\0"):
        if not entry:
            continue
        fields = entry.split(b"\t", 2)
        require(len(fields) == 3 and fields[0].isdigit() and fields[1].isdigit() and fields[2],
                "I refuse a binary, rename or malformed patch entry.")
        names.append(str(relative(fields[2].decode("utf-8", errors="strict"))))
    require(len(names) == len(set(names)), "I refuse duplicate patch targets.")
    return set(names)


def before_ok(raw, row, allow_checkout_eol=True):
    if row.get("before") is None:
        require(raw is None, "I require a new source path to be absent: " + row["path"])
        return
    require(raw is not None, "I require an existing source file: " + row["path"])
    accepted = row.get("accepted_before_raw", [row["before"]])
    require(isinstance(accepted, list) and accepted, "I require explicit accepted-before pins.")
    exact = any(len(raw) == pin.get("bytes") and digest(raw) == pin.get("sha256") for pin in accepted)
    projection = project(raw)
    check_pin(projection, row["before_projection"], row["path"] + " before projection")
    if not exact:
        require(allow_checkout_eol, "I require the exact previous-stage raw bytes: " + row["path"])
        # I accept Git's LF/CRLF checkout conversion only when it reconstructs
        # the exact pinned original through its explicit encoding/EOL recipe.
        recipe = row.get("before_format")
        require(isinstance(recipe, dict), "I require a pinned checkout-EOL recipe.")
        encoding = "utf16le-bom" if raw.startswith(b"\xff\xfe") else (
            "utf8-bom" if raw.startswith(b"\xef\xbb\xbf") else "raw-bytes")
        require(encoding == recipe.get("encoding"), "I refuse a changed source encoding or BOM.")
        check_pin(restore(projection, recipe), row["before"], row["path"] + " reconstructed original")


def atomic_bytes(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=".migration-", dir=path.parent)
    try:
        with os.fdopen(fd, "wb") as f:
            f.write(data)
            f.flush()
            os.fsync(f.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def run_stage(work, bundle, stage, checkout_eol_paths=None):
    expected_patch = set()
    for row in stage["files"]:
        target = no_links(work, row["path"])
        raw = target.read_bytes() if target.is_file() else None
        before_ok(raw, row, allow_checkout_eol=(checkout_eol_paths is None or row["path"] in checkout_eol_paths))
        if raw is not None:
            atomic_bytes(target, project(raw))
        if row.get("before_projection") != row["after_projection"]:
            expected_patch.add(row["path"])
    patch = no_links(bundle, stage["patch"]["path"])
    check_pin(patch.read_bytes(), stage["patch"], stage["id"] + " patch")
    require(patch_membership(work, patch) == expected_patch, "I found an unexpected patch target set.")
    git(work, "apply", "--no-index", "--unidiff-zero", "--whitespace=nowarn", "--check", str(patch))
    git(work, "apply", "--no-index", "--unidiff-zero", "--whitespace=nowarn", str(patch))
    for row in stage["files"]:
        target = no_links(work, row["path"])
        projection = target.read_bytes()
        check_pin(projection, row["after_projection"], row["path"] + " after projection")
        raw = restore(projection, row["after_format"])
        check_pin(raw, row["after"], row["path"] + " after raw")
        atomic_bytes(target, raw)


def initial_files(core, manifest):
    rows = {}
    for stage in manifest["stages"]:
        for row in stage["files"]:
            rows.setdefault(row["path"], row)
    originals = {}
    for name, row in rows.items():
        path = no_links(core, name)
        require(not path.exists() or path.is_file(), "I require selected paths to be regular files.")
        raw = path.read_bytes() if path.is_file() else None
        before_ok(raw, row)
        originals[name] = raw
    return originals


def check_series(core, manifest, bundle):
    originals = initial_files(core, manifest)
    with tempfile.TemporaryDirectory(prefix="opendbo-migration-check-") as tmp:
        work = Path(tmp)
        for name, raw in originals.items():
            if raw is not None:
                atomic_bytes(work / name, raw)
        seen = set()
        for stage in manifest["stages"]:
            paths = {row["path"] for row in stage["files"]}
            run_stage(work, bundle, stage, paths - seen)
            seen.update(paths)
        names = {p.relative_to(work).as_posix() for p in work.rglob("*") if p.is_file()}
        require(names == set(originals), "I found an unexpected staged source membership.")
        final = {name: (work / name).read_bytes() for name in names}
    for name, raw in originals.items():
        path = no_links(core, name)
        require((path.read_bytes() if path.is_file() else None) == raw,
                "I found a checkout change during verification.")
    return originals, final


def apply_series(core, manifest, bundle, branch, originals, final):
    checkout_state(core, expected_branch=branch)
    backup = Path(tempfile.mkdtemp(prefix="opendbo-migration-backup-"))
    known = {name: {digest(raw)} if raw is not None else set() for name, raw in originals.items()}
    for name, raw in originals.items():
        if raw is not None:
            atomic_bytes(backup / name, raw)
            known[name].add(digest(project(raw)))
    for stage in manifest["stages"]:
        for row in stage["files"]:
            known[row["path"]].update((row["after"]["sha256"], row["after_projection"]["sha256"]))
    try:
        require(initial_files(core, manifest) == originals, "I found a changed source before application.")
        seen = set()
        for stage in manifest["stages"]:
            checkout_state(core, expected_branch=branch, allowed_dirty=originals)
            paths = {row["path"] for row in stage["files"]}
            run_stage(core, bundle, stage, paths - seen)
            seen.update(paths)
        for name, raw in final.items():
            require(no_links(core, name).read_bytes() == raw, "I found a final source mismatch.")
        checkout_state(core, expected_branch=branch, allowed_dirty=originals)
    except BaseException as error:
        conflicts = []
        for name, raw in originals.items():
            try:
                path = no_links(core, name)
                current = path.read_bytes() if path.is_file() else None
                require(current is None or digest(current) in known[name], "Unknown concurrent bytes")
                if raw is None:
                    if path.is_file():
                        path.unlink()
                else:
                    atomic_bytes(path, raw)
            except BaseException:
                conflicts.append(name)
        raise Refusal("I stopped application and attempted rollback; backup: %s; unresolved paths: %d."
                      % (backup, len(conflicts))) from error
    return backup


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("core_checkout", type=Path, help="My explicit upstream Core Git checkout; no discovery.")
    parser.add_argument("--manifest", type=Path,
                        default=Path(__file__).resolve().parents[1] / "source-export-manifest.json")
    parser.add_argument("--manifest-sha256", help="My independently checked release-manifest SHA-256.")
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--check", action="store_true", help="Default: only temporary selected-file copies are written.")
    modes.add_argument("--apply", action="store_true", help="Explicitly apply without staging/committing Git changes.")
    parser.add_argument("--branch", help="Exact dedicated x64 working branch required for --apply.")
    parser.add_argument("--stage", choices=("baseline", "all"), default="all",
                        help="I may prepare an independent baseline-only Win32 tree; default applies both stages.")
    args = parser.parse_args(argv)
    try:
        require(not args.apply or (args.branch and args.manifest_sha256),
                "I require --branch and --manifest-sha256 for --apply.")
        core = args.core_checkout.resolve(strict=True)
        manifest_path = args.manifest.resolve(strict=True)
        state = checkout_state(core, expected_branch=args.branch if args.apply else None)
        manifest, raw = load_manifest(manifest_path, args.manifest_sha256)
        if args.stage == "baseline":
            manifest = dict(manifest, stages=manifest["stages"][:1])
        originals, final = check_series(core, manifest, manifest_path.parent)
        backup = None
        if args.apply:
            backup = apply_series(core, manifest, manifest_path.parent, args.branch, originals, final)
        require(manifest_path.read_bytes() == raw, "I found a changed manifest during the operation.")
        print(json.dumps({"state": "SOURCE_APPLIED_NOT_BUILT" if args.apply else "CHECK_PASS_NO_CHECKOUT_WRITES",
                          "base_commit": state["head"], "branch": state["branch"],
                          "manifest_sha256": digest(raw), "stages": [s["id"] for s in manifest["stages"]],
                          "selected_union_files": len(originals), "backup_directory": str(backup) if backup else None,
                          "index_staged": False, "runtime_or_functional_qualification": False}, indent=2))
        return 0
    except (Refusal, OSError, ValueError, UnicodeError, KeyError, TypeError) as error:
        print(json.dumps({"state": "REFUSED_NO_PASS_CLAIM", "reason": str(error)}, ensure_ascii=True), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
