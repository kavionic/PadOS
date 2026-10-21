#!/usr/bin/env python3

# This file is part of PadOS.
#
# Copyright (c) 2026 Kurt Skauen
#
# SPDX-License-Identifier: Apache-2.0
###############################################################################

"""Generate compact Unicode default lower/upper casing tables used by PString."""

import argparse
import hashlib
from pathlib import Path

from generate_unicode_case_folding import UNICODE_VERSION, compress_single_mappings


SOURCE_HASHES = {
    "UnicodeData.txt": "2e1efc1dcb59c575eedf5ccae60f95229f706ee6d031835247d843c11d96470c",
    "SpecialCasing.txt": "efc25faf19de21b92c1194c111c932e03d2a5eaf18194e33f1156e96de4c9588",
    "DerivedCoreProperties.txt": "24c7fed1195c482faaefd5c1e7eb821c5ee1fb6de07ecdbaa64b56a99da22c08",
}
MAX_EXPANSION_LENGTH = 3


def load_source(source_directory: Path, filename: str) -> str:
    source_data = (source_directory / filename).read_bytes()
    source_hash = hashlib.sha256(source_data).hexdigest()
    if source_hash != SOURCE_HASHES[filename]:
        raise RuntimeError(f"Unexpected {filename} SHA-256: {source_hash}")
    return source_data.decode("utf-8")


def load_mappings(source_directory: Path) -> dict[str, dict[int, tuple[int, ...]]]:
    mappings: dict[str, dict[int, tuple[int, ...]]] = {"Lower": {}, "Upper": {}}
    for source_line in load_source(source_directory, "UnicodeData.txt").splitlines():
        fields = source_line.split(";")
        character = int(fields[0], 16)
        for name, field_index in (("Lower", 13), ("Upper", 12)):
            if fields[field_index]:
                mappings[name][character] = (int(fields[field_index], 16),)

    final_sigma_found = False
    for source_line in load_source(source_directory, "SpecialCasing.txt").splitlines():
        data_line = source_line.split("#", 1)[0].strip()
        if not data_line:
            continue
        fields = [field.strip() for field in data_line.split(";")]
        character = int(fields[0], 16)
        conditions = fields[4].split()
        if conditions:
            if conditions == ["Final_Sigma"]:
                if character != 0x03a3 or fields[1] != "03C2" or fields[3] != "03A3":
                    raise RuntimeError("Unexpected final sigma mapping")
                final_sigma_found = True
            elif not any(condition in {"tr", "az", "lt"} for condition in conditions):
                raise RuntimeError(f"Unsupported casing context: {conditions}")
            continue
        for name, field_index in (("Lower", 1), ("Upper", 3)):
            mapping = tuple(int(value, 16) for value in fields[field_index].split())
            if not 1 <= len(mapping) <= MAX_EXPANSION_LENGTH:
                raise RuntimeError(f"Unsupported case mapping for U+{character:04X}")
            if mapping == (character,):
                mappings[name].pop(character, None)
            else:
                mappings[name][character] = mapping
    if not final_sigma_found:
        raise RuntimeError("Missing final sigma mapping")
    return mappings


def load_properties(source_directory: Path) -> dict[str, list[tuple[int, int]]]:
    properties: dict[str, list[tuple[int, int]]] = {"Cased": [], "Case_Ignorable": []}
    for source_line in load_source(source_directory, "DerivedCoreProperties.txt").splitlines():
        data_line = source_line.split("#", 1)[0].strip()
        if not data_line:
            continue
        fields = [field.strip() for field in data_line.split(";")]
        if fields[1] not in properties:
            continue
        endpoints = fields[0].split("..")
        first = int(endpoints[0], 16)
        last = int(endpoints[-1], 16)
        ranges = properties[fields[1]]
        if ranges and first <= ranges[-1][1]:
            raise RuntimeError(f"Overlapping {fields[1]} ranges")
        if ranges and first == ranges[-1][1] + 1:
            ranges[-1] = (ranges[-1][0], last)
        else:
            ranges.append((first, last))
    if not all(properties.values()):
        raise RuntimeError("Missing casing properties")
    return properties


def verify_lowercase_context(mappings: dict[int, tuple[int, ...]], properties: dict[str, list[tuple[int, int]]]) -> None:
    cased = {character for first, last in properties["Cased"] for character in range(first, last + 1)}
    case_ignorable = {character for first, last in properties["Case_Ignorable"] for character in range(first, last + 1)}
    # Include the conditional Final_Sigma mapping validated in load_mappings().
    context_mappings = [*mappings.items(), (0x03a3, (0x03c2,))]
    for character, mapping in context_mappings:
        original_context = [] if character in case_ignorable else [character in cased]
        converted_context = [target in cased for target in mapping if target not in case_ignorable]
        if original_context != converted_context:
            raise RuntimeError(f"Lowercase mapping for U+{character:04X} changes sigma context")


def append_mappings(lines: list[str], name: str, mappings: dict[int, tuple[int, ...]]) -> None:
    # Reuse the case-folding generator's range compression, excluding the ASCII fast path.
    non_ascii_mappings = {source: mapping for source, mapping in mappings.items() if source >= 0x80}
    ranges, exceptions = compress_single_mappings(non_ascii_mappings)
    ranges.extend((source, target - source, 1, 1) for source, target in exceptions.items())
    ranges.sort()
    expansions: list[tuple[int, int, int]] = []
    expansion_characters: list[int] = []
    for source, mapping in sorted(non_ascii_mappings.items()):
        if len(mapping) > 1:
            if len(expansion_characters) > 0xffff:
                raise RuntimeError("Expansion offset exceeds uint16_t")
            expansions.append((source, len(expansion_characters), len(mapping)))
            expansion_characters.extend(mapping)

    generated = {source: mapping for source, mapping in mappings.items() if source < 0x80}
    previous_last = -1
    for first, delta, count, stride in ranges:
        if first <= previous_last:
            raise RuntimeError("Overlapping case mapping ranges")
        previous_last = first + (count - 1) * stride
        for index in range(count):
            source = first + index * stride
            generated[source] = (source + delta,)
    for source, offset, count in expansions:
        if source in generated:
            raise RuntimeError(f"Duplicate mapping for U+{source:04X}")
        generated[source] = tuple(expansion_characters[offset:offset + count])
    if generated != mappings:
        raise RuntimeError(f"Generated {name} mappings differ from the Unicode data")

    lines.extend([f"inline constexpr UnicodeCaseMappingRange UNICODE_{name.upper()}_RANGES[] =", "{"])
    for first, delta, count, stride in ranges:
        lines.append(f"    {{0x{first:06x}u, {delta}, {count}, {stride}}},")
    lines.extend(["};", "", f"inline constexpr UnicodeCaseExpansion UNICODE_{name.upper()}_EXPANSIONS[] =", "{"])
    for source, offset, count in expansions:
        lines.append(f"    {{0x{source:06x}u, {offset}, {count}}},")
    lines.extend(["};", "", f"inline constexpr uint32_t UNICODE_{name.upper()}_EXPANSION_CHARACTERS[] =", "{"])
    for index in range(0, len(expansion_characters), 8):
        lines.append("    " + " ".join(f"0x{character:06x}u," for character in expansion_characters[index:index + 8]))
    lines.extend(["};", ""])


def generate_header(source_directory: Path) -> str:
    mappings = load_mappings(source_directory)
    properties = load_properties(source_directory)
    verify_lowercase_context(mappings["Lower"], properties)
    lines = [
        "// This file is part of PadOS.",
        "//",
        "// Copyright (c) 2026 Kurt Skauen",
        "//",
        "// SPDX-License-Identifier: Apache-2.0",
        "///////////////////////////////////////////////////////////////////////////////",
        "// Generated by Tools/generate_unicode_case_mapping.py. Do not edit.",
    ]
    for filename, source_hash in SOURCE_HASHES.items():
        lines.extend([
            f"// Source: https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/{filename}",
            f"// SHA-256: {source_hash}",
        ])
    lines.extend([
        "// Unicode data copyright (C) 1991-2025 Unicode, Inc.",
        "// Unicode data license: https://www.unicode.org/license.txt",
        "",
        "#pragma once",
        "",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        "struct UnicodeCaseMappingRange",
        "{",
        "    uint32_t First;",
        "    int32_t Delta;",
        "    uint8_t Count;",
        "    uint8_t Stride;",
        "};",
        "",
        "struct UnicodeCaseExpansion",
        "{",
        "    uint32_t Source;",
        "    uint16_t Offset;",
        "    uint8_t Count;",
        "};",
        "",
        "struct UnicodeCasingPropertyRange",
        "{",
        "    uint32_t First;",
        "    uint32_t Last;",
        "};",
        "",
        f"inline constexpr size_t UNICODE_CASE_MAPPING_MAX_EXPANSION_LENGTH = {MAX_EXPANSION_LENGTH};",
        "",
    ])
    for name, mapping in mappings.items():
        append_mappings(lines, name, mapping)
    for name, ranges in properties.items():
        lines.extend([f"inline constexpr UnicodeCasingPropertyRange UNICODE_{name.upper()}_RANGES[] =", "{"])
        for first, last in ranges:
            lines.append(f"    {{0x{first:06x}u, 0x{last:06x}u}},")
        lines.extend(["};", ""])
    return "\r\n".join(lines)


def main() -> None:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("source_directory", type=Path, help=f"directory containing the Unicode {UNICODE_VERSION} data")
    argument_parser.add_argument("output", type=Path, help="output UnicodeCaseMappingData.h")
    arguments = argument_parser.parse_args()
    arguments.output.write_bytes(generate_header(arguments.source_directory).encode("utf-8"))


if __name__ == "__main__":
    main()
