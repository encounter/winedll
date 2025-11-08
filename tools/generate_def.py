#!/usr/bin/env python3
"""Generate a Windows .def file from a Wine .spec file."""

from __future__ import annotations

import argparse
import os
from typing import List, Tuple

from spec_parser import (
    ALL_ARCHES,
    SpecEntry,
    make_c_identifier,
    normalize_arch_name,
    read_spec_file,
)


def select_exports(
    entries: List[SpecEntry], target_arch: str
) -> List[Tuple[SpecEntry, int]]:
    """Filter spec entries for the requested architecture while preserving order."""
    selected: List[Tuple[SpecEntry, int]] = []
    ordinal = 1

    for entry in entries:
        if entry.entry_type is None:
            continue
        if not entry.function_name:
            continue
        if not entry.matches_arch(target_arch):
            continue

        selected.append((entry, ordinal))
        ordinal += 1

    return selected


def format_export_line(
    entry: SpecEntry,
    ordinal: int,
    arch: str,
    imports_only: bool,
    no_stdcall_suffix: bool,
) -> str:
    """Format a DEF export line for the given entry and ordinal."""
    if not entry.function_name:
        raise ValueError("Spec entry is missing a function name.")

    internal_name = entry.internal_name or entry.function_name
    if entry.entry_type == "stub":
        internal_name = make_c_identifier(internal_name)

    line = f"  {entry.function_name}"
    if entry.entry_type == "stdcall" and not no_stdcall_suffix:
        size = entry.arguments_size(arch)
        line += f"@{size}"
    if not imports_only and internal_name != entry.function_name:
        line += f"={internal_name}"
    line += f" @{ordinal}"

    if entry.entry_type == "extern":
        line += " DATA"

    if any(modifier == "-private" for modifier in entry.modifiers):
        line += " PRIVATE"

    return line


def generate_def_file(
    spec_file: str,
    output_file: str,
    arch: str,
    imports_only: bool,
    no_stdcall_suffix: bool,
) -> None:
    """Generate the DEF file contents and write them to disk."""
    entries = read_spec_file(spec_file)
    target_arch = normalize_arch_name(arch)
    exports = select_exports(entries, target_arch)

    library_name = os.path.splitext(os.path.basename(spec_file))[0]
    if not library_name:
        library_name = "library"
    library_line = f"LIBRARY {library_name}.dll"
    comment_line = f"; File generated automatically from {os.path.abspath(spec_file)}; do not edit!"

    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)

    with open(output_file, "w", encoding="utf-8") as handle:
        handle.write(comment_line)
        handle.write("\n\n")
        handle.write(library_line)
        handle.write("\n\n")
        handle.write("EXPORTS\n")
        for entry, ordinal in exports:
            handle.write(
                format_export_line(
                    entry, ordinal, arch, imports_only, no_stdcall_suffix
                )
            )
            handle.write("\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a Windows .def file from a Wine .spec file."
    )
    parser.add_argument("spec_file", help="Path to the input .spec file")
    parser.add_argument("output_file", help="Path to the output .def file")
    parser.add_argument(
        "--arch",
        help="Target architecture to filter exports (e.g. i386, x86_64, arm64).",
        required=True,
    )
    parser.add_argument(
        "--imports-only",
        action="store_true",
        help="Generate an importlib compatible .def file",
    )
    parser.add_argument(
        "--no-stdcall-suffix",
        action="store_true",
        help="Disable stdcall suffix (@)",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.arch not in ALL_ARCHES:
        raise ValueError(f"Unsupported architecture: {args.arch}")
    generate_def_file(
        args.spec_file,
        args.output_file,
        args.arch,
        args.imports_only,
        args.no_stdcall_suffix,
    )


if __name__ == "__main__":
    main()
