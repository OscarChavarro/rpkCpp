#!/usr/bin/env python3
import argparse
import csv
import re
import shutil
import subprocess
import sys
from pathlib import Path


def norm_key(path: Path) -> str:
    return str(path.resolve()).lower()


def with_upper_dirs(path_str: str) -> str:
    parts = Path(path_str.replace("\\", "/")).parts
    if len(parts) <= 1:
        return path_str.replace("\\", "/")
    return "/".join([* [p.upper() for p in parts[:-1]], parts[-1]])


def load_mappings(csv_path: Path):
    rows = []
    with csv_path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        required = {"old_path", "new_path_8_3"}
        if not required.issubset(set(reader.fieldnames or [])):
            missing = required - set(reader.fieldnames or [])
            raise ValueError("Missing columns in mappings.csv: " + ", ".join(sorted(missing)))
        for row in reader:
            old_path = row["old_path"]
            new_path = with_upper_dirs(row["new_path_8_3"])
            if old_path.startswith("src/render/opengl/") or new_path.startswith("src/render/opengl/"):
                continue
            rows.append((new_path, old_path))
    return rows


INCLUDE_RE = re.compile(r'^(\s*#\s*include\s*)"([^"]+)"(.*)$')


def build_include_maps(mappings):
    exact = {}
    dir_exact = {}
    basename = {}
    collisions = set()
    for old_rel, new_rel in mappings:
        old_posix = old_rel.replace("\\", "/")
        new_posix = new_rel.replace("\\", "/")
        exact[old_posix.lower()] = new_posix
        old_dir_parts = Path(old_posix).parts[:-1]
        new_dir_parts = Path(new_posix).parts[:-1]
        for depth in range(1, min(len(old_dir_parts), len(new_dir_parts)) + 1):
            dir_exact["/".join(old_dir_parts[:depth]).lower()] = "/".join(new_dir_parts[:depth])

        old_name = Path(old_posix).name
        new_name = Path(new_posix).name
        old_name_key = old_name.lower()
        existing = basename.get(old_name_key)
        if existing is None:
            basename[old_name_key] = new_name
        elif existing != new_name:
            collisions.add(old_name_key)

    for name in collisions:
        basename.pop(name, None)
    return exact, dir_exact, basename


def rewrite_include_target(target: str, exact_map, dir_map, basename_map):
    normalized = target.replace("\\", "/")
    dot_prefix = ""
    while normalized.startswith("./"):
        dot_prefix += "./"
        normalized = normalized[2:]

    replacement = exact_map.get(normalized.lower())
    if replacement is None and normalized.startswith("src/"):
        replacement = exact_map.get(normalized[4:].lower())
    if replacement is None and not normalized.startswith("src/"):
        replacement = exact_map.get(f"src/{normalized}".lower())
        if replacement and replacement.lower().startswith("src/"):
            replacement = replacement[4:]

    if replacement is None and "/" in normalized:
        parent, file_name = normalized.rsplit("/", 1)
        mapped_parent = dir_map.get(parent.lower())
        if mapped_parent is None and parent.startswith("src/"):
            mapped_parent = dir_map.get(parent[4:].lower())
        if mapped_parent is None and not parent.startswith("src/"):
            mapped_parent = dir_map.get(f"src/{parent}".lower())
            if mapped_parent and mapped_parent.lower().startswith("src/"):
                mapped_parent = mapped_parent[4:]
        if mapped_parent:
            replacement = f"{mapped_parent}/{file_name}"

    if replacement is None:
        name = Path(normalized).name
        mapped_name = basename_map.get(name.lower())
        if mapped_name:
            if "/" in normalized:
                parent = normalized.rsplit("/", 1)[0]
                replacement = f"{parent}/{mapped_name}"
            else:
                replacement = mapped_name

    if replacement is None:
        return None

    return f"{dot_prefix}{replacement}"


def update_quoted_includes(project_root: Path, mappings, dry_run: bool):
    exact_map, dir_map, basename_map = build_include_maps(mappings)
    files_changed = 0
    includes_changed = 0

    for source_file in sorted(project_root.rglob("*")):
        if source_file.suffix.lower() not in {".cpp", ".h"}:
            continue
        if not source_file.is_file():
            continue

        original = source_file.read_text(encoding="utf-8")
        new_lines = []
        file_changed = False

        for lineno, line in enumerate(original.splitlines(keepends=True), start=1):
            line_ending = ""
            if line.endswith("\r\n"):
                line_ending = "\r\n"
            elif line.endswith("\n"):
                line_ending = "\n"

            match = INCLUDE_RE.match(line)
            if not match:
                new_lines.append(line)
                continue

            prefix, include_target, suffix = match.groups()
            replacement = rewrite_include_target(include_target, exact_map, dir_map, basename_map)
            if not replacement or replacement == include_target:
                new_lines.append(line)
                continue

            suffix_no_eol = suffix
            if line_ending and suffix.endswith(line_ending):
                suffix_no_eol = suffix[: -len(line_ending)]

            new_line = f'{prefix}"{replacement}"{suffix_no_eol}{line_ending}'
            new_lines.append(new_line)
            file_changed = True
            includes_changed += 1
            print(f'  [include] {source_file.relative_to(project_root)}:{lineno}: "{include_target}" -> "{replacement}"')

        if file_changed:
            files_changed += 1
            if not dry_run:
                source_file.write_text("".join(new_lines), encoding="utf-8")

    action = "Would update" if dry_run else "Updated"
    print(f"{action} quoted includes in {files_changed} files ({includes_changed} includes).")


def convert_to_unix_text(project_root: Path, dry_run: bool):
    cmd = shutil.which("fromdos")
    if cmd is None:
        print('[warn] "fromdos" not found in PATH; skipping Unix text conversion.')
        return

    targets = [p for p in sorted(project_root.rglob("*")) if p.is_file() and p.suffix.lower() in {".cpp", ".h"}]
    if not targets:
        print("No .cpp/.h files found for Unix text conversion.")
        return

    print(f'Phase 4: converting {len(targets)} files to Unix text with "fromdos"')
    if dry_run:
        for path in targets:
            print(f"  [dry-run] fromdos {path.relative_to(project_root)}")
        return

    for path in targets:
        subprocess.run([cmd, str(path)], check=True)


def rename_with_temp(src: Path, dst: Path, dry_run: bool, tag: str):
    if src == dst:
        return

    # Allow case-only rename on case-insensitive filesystems by using temp names.
    if dst.exists() and src.name.lower() != dst.name.lower():
        raise FileExistsError(f"Destination already exists: {dst}")

    tmp = src.with_name(f"{src.name}.maptmp_{tag}")
    idx = 1
    while tmp.exists():
        idx += 1
        tmp = src.with_name(f"{src.name}.maptmp_{tag}_{idx}")

    print(f"  {src} -> {tmp} -> {dst}")
    if not dry_run:
        src.rename(tmp)
        tmp.rename(dst)


def build_dir_restore_map(mappings):
    restore_map = {}
    for _src_83, old_rel in mappings:
        old_parts = Path(old_rel.replace("\\", "/")).parts[:-1]
        if not old_parts:
            continue
        for depth in range(1, len(old_parts) + 1):
            old_dir = old_parts[:depth]
            upper_key = "/".join(old_dir).upper()
            restore_map[upper_key] = old_dir[-1]
    return restore_map


def restore_directory_casing(project_root: Path, mappings, dry_run: bool):
    restore_map = build_dir_restore_map(mappings)
    dirs = [p for p in project_root.rglob("*") if p.is_dir()]
    dirs.sort(key=lambda p: len(p.relative_to(project_root).parts), reverse=True)

    candidates = []
    for src in dirs:
        if src == project_root:
            continue
        rel = src.relative_to(project_root).as_posix()
        target_name = restore_map.get(rel.upper())
        if not target_name or target_name == src.name:
            continue
        dst = src.with_name(target_name)
        candidates.append((src, dst))

    if not candidates:
        print("Phase 5: no directories needed casing restore.")
        return

    print(f"Phase 5: restoring casing for {len(candidates)} directories")
    for src, dst in candidates:
        rename_with_temp(src, dst, dry_run, "dircase")


def apply_from_83(project_root: Path, mappings, dry_run: bool, strict: bool):
    steps = []
    for old_rel, new_rel in mappings:
        src = project_root / old_rel
        dst = project_root / new_rel
        if src == dst:
            continue
        if not src.exists():
            if strict:
                raise FileNotFoundError(f"Source not found: {src}")
            print(f"[skip] missing source: {old_rel}")
            continue
        steps.append((src, dst))

    if not steps:
        print("No files to move.")
        return

    source_keys = {norm_key(src) for src, _dst in steps}

    temp_moves = []
    for idx, (src, _dst) in enumerate(steps, start=1):
        tmp = src.with_name(f"{src.name}.maptmp_{idx}")
        while tmp.exists():
            idx += 1
            tmp = src.with_name(f"{src.name}.maptmp_{idx}")
        temp_moves.append((src, tmp))

    print(f"Phase 1: moving {len(temp_moves)} sources to temp names")
    for src, tmp in temp_moves:
        print(f"  {src} -> {tmp}")
        if not dry_run:
            src.rename(tmp)

    print("Phase 2: moving temp names back to original names")
    for (src, dst), (_src2, tmp) in zip(steps, temp_moves):
        if dst.exists() and norm_key(dst) not in source_keys:
            raise FileExistsError(f"Destination already exists: {dst}")
        print(f"  {tmp} -> {dst}")
        if not dry_run:
            dst.parent.mkdir(parents=True, exist_ok=True)
            tmp.rename(dst)

    print(f"Done. Files moved: {len(steps)}")


def main():
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    parser = argparse.ArgumentParser(description="Apply mappings.csv from 8.3 names back to normal names")
    parser.add_argument("--csv", default=str(project_root / "mappings.csv"), help="Path to mappings.csv")
    parser.add_argument("--dry-run", action="store_true", help="Print moves without applying them")
    parser.add_argument("--strict", action="store_true", help="Fail if any source file is missing")
    args = parser.parse_args()

    csv_path = Path(args.csv).resolve()
    if not csv_path.exists():
        print(f"mappings.csv not found: {csv_path}", file=sys.stderr)
        return 2

    mappings = load_mappings(csv_path)
    apply_from_83(project_root, mappings, args.dry_run, args.strict)
    print("Phase 3: updating quoted #include directives")
    update_quoted_includes(project_root, mappings, args.dry_run)
    convert_to_unix_text(project_root, args.dry_run)
    restore_directory_casing(project_root, mappings, args.dry_run)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
