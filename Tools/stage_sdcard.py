#!/usr/bin/env python3
# This file is part of PadOS.
# Copyright (c) 2026 Kurt Skauen
# SPDX-License-Identifier: Apache-2.0

"""Merge ordered SD-card source trees. Later sources override earlier sources.

Only files recorded in the external state file are removed when their source
disappears. Unchanged files retain their timestamps. Sync control files may be
edited by RainbowControl; concurrent source edits require explicit resolution.
Unchanged size and modification time reuse cached hashes; --verify checks all
contents, including edits that preserve both size and timestamp.
The caller must serialize invocations sharing an output tree.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat


EXCLUDED_NAMES = frozenset(("CMakeLists.txt", ".git", ".gitignore", "__pycache__"))
CONTROL_NAMES = frozenset((".syncignore", ".deleterules"))


def output_path(output, relative):
    relative_path = Path(relative)
    if relative_path.is_absolute() or ".." in relative_path.parts or relative_path == Path("."):
        raise ValueError(f"Invalid staged path: {relative}")
    target = output / relative_path
    if not target.resolve().is_relative_to(output):
        raise ValueError(f"Staged path escapes output directory: {target}")
    return target


def collect_sources(output, sources):
    files = {}
    spelling = {}
    for source_name, prefix_name in sources:
        source = Path(source_name).resolve()
        prefix = Path(prefix_name)
        if prefix.is_absolute() or ".." in prefix.parts:
            raise ValueError(f"Invalid destination prefix: {prefix}")
        if not source.is_dir():
            raise ValueError(f"Missing SD-card source: {source}")
        if output.is_relative_to(source) or source.is_relative_to(output):
            raise ValueError(f"Source and output directories overlap: {source}")
        for directory, directories, filenames in os.walk(source):
            directories[:] = sorted(name for name in directories if name not in EXCLUDED_NAMES)
            for name in directories + filenames:
                if (Path(directory) / name).is_symlink():
                    raise ValueError(f"SD-card sources must contain regular files and directories: {Path(directory) / name}")
            for name in sorted(filenames):
                if name in EXCLUDED_NAMES:
                    continue
                path = Path(directory) / name
                relative = (prefix / path.relative_to(source)).as_posix()
                key = relative.casefold()
                if key in spelling and spelling[key] != relative:
                    raise ValueError(f"SD-card paths differ only in case: {spelling[key]} and {relative}")
                spelling[key] = relative
                files[relative] = path
    for relative in files:
        for parent in Path(relative).parents:
            if parent.as_posix().casefold() in spelling:
                raise ValueError(f"SD-card path is both a file and directory: {parent}")
    return files


def file_hash(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def file_metadata(path):
    try:
        info = path.stat()
    except FileNotFoundError:
        return None
    if not stat.S_ISREG(info.st_mode):
        raise ValueError(f"Expected a regular file: {path}")
    return [info.st_size, info.st_mtime_ns]


def remove_staged_file(output, relative):
    target = output_path(output, relative)
    if target.is_file():
        target.unlink()
    parent = target.parent
    while parent != output and parent.is_dir():
        try:
            parent.rmdir()
        except OSError:
            break
        parent = parent.parent


def stage_files(output, state_file, sources, verify=False):
    output = output.resolve()
    state_file = state_file.resolve()
    if state_file.is_relative_to(output):
        raise ValueError("The staging state file must be outside the SD-card tree.")
    files = collect_sources(output, sources)
    previous = json.loads(state_file.read_text(encoding="utf-8")) if state_file.exists() else {}
    records = {}
    updates = []
    for relative, source in files.items():
        target = output_path(output, relative)
        previous_record = previous.get(relative, {})
        if isinstance(previous_record, str):
            previous_record = {"sha256": previous_record}
        record = {
            "source": str(source),
            "source_metadata": file_metadata(source),
            "target_metadata": file_metadata(target),
        }
        records[relative] = record
        control_file = source.name in CONTROL_NAMES
        if (not verify and not control_file and record["target_metadata"] is not None
                and previous_record.get("sha256")
                and all(previous_record.get(key) == value for key, value in record.items())):
            record["sha256"] = previous_record["sha256"]
            continue
        record["sha256"] = file_hash(source)
        target_hash = file_hash(target) if record["target_metadata"] is not None else None
        if target_hash == record["sha256"]:
            continue
        if control_file and target_hash is not None:
            if relative not in previous or record["sha256"] == previous_record["sha256"]:
                continue
            if target_hash != previous_record["sha256"]:
                raise ValueError(f"Both source and staged sync control file changed; reconcile them before building: {target}")
        updates.append((relative, source, target))
    removed = [
        relative for relative in previous
        if relative not in files and Path(relative).name not in CONTROL_NAMES
    ]
    # Validate every old path before modifying the output tree.
    for relative in removed:
        output_path(output, relative)
    output.mkdir(parents=True, exist_ok=True)
    state_file.parent.mkdir(parents=True, exist_ok=True)
    temporary = state_file.with_suffix(".copy.tmp")
    try:
        for relative, source, target in updates:
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, temporary)
            temporary.replace(target)
            records[relative]["target_metadata"] = file_metadata(target)
    finally:
        temporary.unlink(missing_ok=True)
    for relative in removed:
        remove_staged_file(output, relative)
    contents = json.dumps(records, indent=2, sort_keys=True) + "\n"
    if not state_file.exists() or state_file.read_text(encoding="utf-8") != contents:
        temporary_state = state_file.with_suffix(".tmp")
        temporary_state.write_text(contents, encoding="utf-8")
        temporary_state.replace(state_file)
    print(f"SD card: {len(files)} source files, {len(updates)} updated, {len(removed)} removed.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--state", required=True, type=Path)
    parser.add_argument("--source", required=True, action="append", nargs=2, metavar=("DIRECTORY", "DESTINATION_PREFIX"))
    parser.add_argument("--verify", action="store_true", help="Hash all files, even when size and timestamp are unchanged")
    args = parser.parse_args()
    stage_files(args.output, args.state, args.source, verify=args.verify)


if __name__ == "__main__":
    main()
