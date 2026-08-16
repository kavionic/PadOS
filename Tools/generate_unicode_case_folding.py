#!/usr/bin/env python3

"""Generate the compact Unicode case-folding tables used by PString."""

import argparse
import hashlib
from pathlib import Path


UNICODE_VERSION = "17.0.0"
CASE_FOLDING_SHA256 = "ff8d8fefbf123574205085d6714c36149eb946d717a0c585c27f0f4ef58c4183"
CASE_FOLDING_URL = f"https://www.unicode.org/Public/{UNICODE_VERSION}/ucd/CaseFolding.txt"
CASE_FOLD_EXPANSION_FLAG = 0x80000000
CASE_FOLD_EXPANSION_LENGTH_MASK = 0x00000003
CASE_FOLD_MAX_EXPANSION_LENGTH = 3


def load_case_folding(source_path: Path) -> dict[int, tuple[int, ...]]:
    source_data = source_path.read_bytes()
    source_hash = hashlib.sha256(source_data).hexdigest()
    if source_hash != CASE_FOLDING_SHA256:
        raise RuntimeError(f"Unexpected CaseFolding.txt SHA-256: {source_hash}")

    source_text = source_data.decode("utf-8")
    if f"CaseFolding-{UNICODE_VERSION}.txt" not in source_text[:512]:
        raise RuntimeError(f"The input is not Unicode {UNICODE_VERSION} CaseFolding.txt")

    mappings: dict[int, tuple[int, ...]] = {}
    for source_line in source_text.splitlines():
        data_line = source_line.split("#", 1)[0].strip()
        if not data_line:
            continue

        fields = [field.strip() for field in data_line.split(";")]
        status = fields[1]
        if status not in {"C", "F"}:
            continue

        source_character = int(fields[0], 16)
        folded_characters = tuple(int(character, 16) for character in fields[2].split())
        if source_character in mappings:
            raise RuntimeError(f"Duplicate default mapping for U+{source_character:04X}")
        if len(folded_characters) > CASE_FOLD_MAX_EXPANSION_LENGTH:
            raise RuntimeError(f"Case fold for U+{source_character:04X} is too long")
        mappings[source_character] = folded_characters

    return mappings


def compress_single_mappings(mappings: dict[int, tuple[int, ...]]) -> tuple[list[tuple[int, int, int, int]], dict[int, int]]:
    single_mappings = [
        (source, folded[0])
        for source, folded in sorted(mappings.items())
        if len(folded) == 1 and not 0x41 <= source <= 0x5a
    ]

    ranges: list[tuple[int, int, int, int]] = []
    exceptions: dict[int, int] = {}
    mapping_index = 0
    while mapping_index < len(single_mappings):
        source_character, folded_character = single_mappings[mapping_index]
        delta = folded_character - source_character
        range_end = mapping_index + 1
        stride = 0

        if range_end < len(single_mappings):
            next_source, next_folded = single_mappings[range_end]
            stride = next_source - source_character
            if stride in {1, 2} and next_folded - next_source == delta:
                range_end += 1
                while range_end < len(single_mappings):
                    candidate_source, candidate_folded = single_mappings[range_end]
                    previous_source = single_mappings[range_end - 1][0]
                    if candidate_source - previous_source != stride or candidate_folded - candidate_source != delta:
                        break
                    range_end += 1

        range_length = range_end - mapping_index
        if range_length >= 2:
            if range_length > 255:
                raise RuntimeError(f"Case-fold range at U+{source_character:04X} is too long")
            ranges.append((source_character, delta, range_length, stride))
            mapping_index = range_end
        else:
            exceptions[source_character] = folded_character
            mapping_index += 1

    return ranges, exceptions


def create_exact_mappings(
    mappings: dict[int, tuple[int, ...]],
    single_exceptions: dict[int, int],
) -> tuple[list[tuple[int, int]], list[int]]:
    encoded_mappings = dict(single_exceptions)
    expansion_characters: list[int] = []

    for source_character, folded_characters in sorted(mappings.items()):
        if len(folded_characters) == 1:
            continue

        expansion_offset = len(expansion_characters)
        if expansion_offset > 0x1fffffff:
            raise RuntimeError("The expansion table offset does not fit in the encoding")
        encoded_mapping = CASE_FOLD_EXPANSION_FLAG | (expansion_offset << 2) | len(folded_characters)
        encoded_mappings[source_character] = encoded_mapping
        expansion_characters.extend(folded_characters)

    return sorted(encoded_mappings.items()), expansion_characters


def decode_exact_mapping(encoded_mapping: int, expansion_characters: list[int]) -> tuple[int, ...]:
    if (encoded_mapping & CASE_FOLD_EXPANSION_FLAG) == 0:
        return (encoded_mapping,)

    expansion_length = encoded_mapping & CASE_FOLD_EXPANSION_LENGTH_MASK
    expansion_offset = (
        encoded_mapping & ~(CASE_FOLD_EXPANSION_FLAG | CASE_FOLD_EXPANSION_LENGTH_MASK)
    ) >> 2
    return tuple(expansion_characters[expansion_offset : expansion_offset + expansion_length])


def verify_generated_tables(
    mappings: dict[int, tuple[int, ...]],
    ranges: list[tuple[int, int, int, int]],
    exact_mappings: list[tuple[int, int]],
    expansion_characters: list[int],
) -> None:
    generated_mappings = {source: (source + 0x20,) for source in range(0x41, 0x5b)}

    for first_character, delta, count, stride in ranges:
        for range_index in range(count):
            source_character = first_character + range_index * stride
            if source_character in generated_mappings:
                raise RuntimeError(f"Duplicate generated mapping for U+{source_character:04X}")
            generated_mappings[source_character] = (source_character + delta,)

    for source_character, encoded_mapping in exact_mappings:
        if source_character in generated_mappings:
            raise RuntimeError(f"Duplicate generated mapping for U+{source_character:04X}")
        generated_mappings[source_character] = decode_exact_mapping(encoded_mapping, expansion_characters)

    if generated_mappings != mappings:
        missing_characters = sorted(set(mappings) ^ set(generated_mappings))
        if missing_characters:
            raise RuntimeError(f"Generated mapping set differs at U+{missing_characters[0]:04X}")
        for source_character, folded_characters in mappings.items():
            if generated_mappings[source_character] != folded_characters:
                raise RuntimeError(f"Generated mapping differs at U+{source_character:04X}")


def append_header(lines: list[str]) -> None:
    lines.extend(
        [
            "// This file is part of PadOS.",
            "//",
            "// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>",
            "//",
            "// PadOS is free software : you can redistribute it and / or modify",
            "// it under the terms of the GNU General Public License as published by",
            "// the Free Software Foundation, either version 3 of the License, or",
            "// (at your option) any later version.",
            "//",
            "// PadOS is distributed in the hope that it will be useful,",
            "// but WITHOUT ANY WARRANTY; without even the implied warranty of",
            "// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the",
            "// GNU General Public License for more details.",
            "//",
            "// You should have received a copy of the GNU General Public License",
            "// along with PadOS. If not, see <http://www.gnu.org/licenses/>.",
            "///////////////////////////////////////////////////////////////////////////////",
            "// Generated by Tools/generate_unicode_case_folding.py. Do not edit.",
            f"// Source: {CASE_FOLDING_URL}",
            f"// SHA-256: {CASE_FOLDING_SHA256}",
            "// Unicode data copyright (C) 1991-2025 Unicode, Inc.",
            "// Unicode data license: https://www.unicode.org/license.txt",
            "",
        ]
    )


def generate_header(
    ranges: list[tuple[int, int, int, int]],
    exact_mappings: list[tuple[int, int]],
    expansion_characters: list[int],
) -> str:
    lines: list[str] = []
    append_header(lines)
    lines.extend(
        [
            "#pragma once",
            "",
            "#include <cstdint>",
            "",
            "struct UnicodeCaseFoldRange",
            "{",
            "    uint32_t First;",
            "    int32_t Delta;",
            "    uint8_t Count;",
            "    uint8_t Stride;",
            "};",
            "",
            "struct UnicodeCaseFoldMapping",
            "{",
            "    uint32_t Source;",
            "    uint32_t Mapping;",
            "};",
            "",
            f"inline constexpr uint32_t UNICODE_CASE_FOLD_EXPANSION_FLAG = 0x{CASE_FOLD_EXPANSION_FLAG:08x}u;",
            f"inline constexpr uint32_t UNICODE_CASE_FOLD_EXPANSION_LENGTH_MASK = 0x{CASE_FOLD_EXPANSION_LENGTH_MASK:08x}u;",
            "",
            "inline constexpr UnicodeCaseFoldRange g_UnicodeCaseFoldRanges[] =",
            "{",
        ]
    )
    for first_character, delta, count, stride in ranges:
        lines.append(f"    {{0x{first_character:06x}u, {delta}, {count}, {stride}}},")
    lines.extend(
        [
            "};",
            "",
            "inline constexpr UnicodeCaseFoldMapping g_UnicodeCaseFoldMappings[] =",
            "{",
        ]
    )
    for source_character, encoded_mapping in exact_mappings:
        lines.append(f"    {{0x{source_character:06x}u, 0x{encoded_mapping:08x}u}},")
    lines.extend(
        [
            "};",
            "",
            "inline constexpr uint32_t g_UnicodeCaseFoldExpansionCharacters[] =",
            "{",
        ]
    )
    for character_index in range(0, len(expansion_characters), 8):
        characters = expansion_characters[character_index : character_index + 8]
        lines.append("    " + " ".join(f"0x{character:06x}u," for character in characters))
    lines.extend(["};", ""])
    return "\r\n".join(lines)


def main() -> None:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("source", type=Path, help="Unicode CaseFolding.txt")
    argument_parser.add_argument("output", type=Path, help="output UnicodeCaseFoldingData.h")
    arguments = argument_parser.parse_args()

    mappings = load_case_folding(arguments.source)
    ranges, single_exceptions = compress_single_mappings(mappings)
    exact_mappings, expansion_characters = create_exact_mappings(mappings, single_exceptions)
    verify_generated_tables(mappings, ranges, exact_mappings, expansion_characters)

    arguments.output.write_bytes(generate_header(ranges, exact_mappings, expansion_characters).encode("utf-8"))


if __name__ == "__main__":
    main()
