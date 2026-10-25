#!/usr/bin/env python3
# This file is part of PadOS.
# Copyright (c) 2026 Kurt Skauen
# SPDX-License-Identifier: Apache-2.0

"""Archive a PadOS kernel/application profile pair and generate gprof reports.

Run from the profiles directory with a prefix or the two profile filenames:
    process_profile.py 261023_024434
    process_profile.py 261023_024434-kernel.gmon 261023_024434-application.gmon

--output overrides PADOS_PROFILE_OUTPUT; the default is the current directory.
Call-graph records select STM32H7-Profile; otherwise STM32H7-Debug is assumed.
An instrumented capture with no recorded calls is indistinguishable from a
sampling-only capture. Use --build Profile for that case.
The current ELF files must still match the firmware used for the capture.
"""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


ELF_NAMES = {"kernel": "PadOS_KernelImage.elf", "application": "Rainbow3DFirmware.elf"}


def shell_path(value):
    # Native Windows Python also needs MSYS paths in environment variables.
    if os.name == "nt" and value.startswith("/"):
        value = subprocess.check_output(["cygpath", "-w", value], text=True).strip()
    return Path(value)


def profile_has_call_graph(path):
    # Parse the 32-bit little-endian records written by KGProfWriteImage().
    data = path.read_bytes()
    if len(data) < 20 or data[:4] != b"gmon" or int.from_bytes(data[4:8], "little") != 1:
        raise ValueError(f"Invalid PadOS gmon header: {path}")
    offset = 20
    has_histogram = False
    has_call_graph = False
    while offset < len(data):
        tag = data[offset]
        offset += 1
        if tag == 0:
            if offset + 32 > len(data):
                raise ValueError(f"Truncated histogram header: {path}")
            bin_count = int.from_bytes(data[offset + 8:offset + 12], "little")
            offset += 32 + bin_count * 2
            has_histogram = True
        elif tag == 1:
            offset += 12
            has_call_graph = True
        else:
            raise ValueError(f"Unsupported gmon record tag {tag}: {path}")
        if offset > len(data):
            raise ValueError(f"Truncated gmon record: {path}")
    if not has_histogram:
        raise ValueError(f"No histogram records in profile: {path}")
    return has_call_graph


def find_gprof():
    executable = shutil.which("arm-unknown-pados-eabi-gprof")
    if executable is None and os.environ.get("PADOS_TOOLCHAIN_PATH"):
        executable = shutil.which(
            "arm-unknown-pados-eabi-gprof",
            path=str(shell_path(os.environ["PADOS_TOOLCHAIN_PATH"]) / "bin"))
    if executable is None:
        raise ValueError("Cannot find arm-unknown-pados-eabi-gprof; add it to PATH or set PADOS_TOOLCHAIN_PATH.")
    return executable


def process_profile(args):
    requested = shell_path(args.profile)
    suffix = "-kernel.gmon"
    prefix = requested.name[:-len(suffix)] if requested.name.endswith(suffix) else requested.name
    if not prefix or prefix in (".", ".."):
        raise ValueError("Specify a nonempty profile prefix or a PREFIX-kernel.gmon filename.")
    profiles = {
        "kernel": requested.with_name(prefix + suffix),
        "application": shell_path(args.application) if args.application else requested.with_name(prefix + "-application.gmon"),
    }
    if profiles["application"].name != prefix + "-application.gmon":
        raise ValueError("Kernel and application filenames must have the same profile prefix.")
    call_graphs = {image: profile_has_call_graph(path) for image, path in profiles.items()}
    has_call_graph = any(call_graphs.values())
    build = args.build
    if build == "auto":
        build = "Profile" if has_call_graph else "Debug"
        if not has_call_graph:
            print(
                "No call-graph records: assuming Debug. Use --build Profile if this was an empty instrumented capture.",
                file=sys.stderr)
    elif build == "Debug" and has_call_graph:
        raise ValueError("The profiles contain call-graph records; use --build Profile.")

    firmware_root = Path(__file__).resolve().parents[2]
    build_directory = firmware_root / "Build_Firmware" / ("STM32H7-" + build) / "Firmware"
    for filename in ELF_NAMES.values():
        if not (build_directory / filename).is_file():
            raise FileNotFoundError(f"Missing ELF: {build_directory / filename}")
    gprof = find_gprof()
    output = shell_path(args.output).resolve()
    destination = output / prefix
    if destination.exists():
        raise FileExistsError(f"Output folder already exists: {destination}")
    output.mkdir(parents=True, exist_ok=True)
    print(f"Using STM32H7-{build}: {build_directory}", flush=True)
    with tempfile.TemporaryDirectory(prefix=f".{prefix}-", dir=output) as temporary:
        staging = Path(temporary)
        for image in ELF_NAMES:
            shutil.copy2(profiles[image], staging / f"{image}.gmon")
            print(f"Copying {ELF_NAMES[image]}...", flush=True)
            shutil.copy2(build_directory / ELF_NAMES[image], staging / ELF_NAMES[image])
        for image in ELF_NAMES:
            print(f"Generating {image}.txt...", flush=True)
            command = [gprof, "--demangle", "--flat-profile"]
            if call_graphs[image]:
                command.append("--graph")
            command.extend([str(staging / ELF_NAMES[image]), str(staging / f"{image}.gmon")])
            with (staging / f"{image}.txt").open("wb") as report:
                subprocess.run(command, stdout=report, check=True)
        staging.rename(destination)
    print(f"Created {destination}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("profile", help="Profile prefix or PREFIX-kernel.gmon filename")
    parser.add_argument("application", nargs="?", help="Matching PREFIX-application.gmon filename; inferred when omitted")
    parser.add_argument("-o", "--output", default=os.environ.get("PADOS_PROFILE_OUTPUT") or ".", help="Archive parent directory")
    parser.add_argument("--build", choices=("auto", "Debug", "Profile"), default="auto", help="ELF build (default: auto)")
    args = parser.parse_args()
    try:
        process_profile(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")


if __name__ == "__main__":
    main()
