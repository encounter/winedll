#!/usr/bin/env python3
"""Generate C stub implementations from Wine .spec stub entries."""

from __future__ import annotations

import argparse
from typing import List

from spec_parser import (
    ALL_ARCHES,
    TYPE_MAP,
    SpecEntry,
    make_c_identifier,
    read_spec_file,
)

STUB_HEADER = """/*
 * msvcrt stub implementations
 *
 * Auto-generated file - DO NOT EDIT
 * Generated from spec file stub entries
 */

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

/* Helper macro for unimplemented functions */
#define MSVCRT_UNIMPLEMENTED(name) \\
    do { \\
        static int once = 0; \\
        if (!once++) { \\
            FIXME("%s: stub\\n", name); \\
        } \\
    } while(0)

"""


def select_stub_entries(entries: List[SpecEntry], target_arch: str) -> List[SpecEntry]:
    """Filter spec entries for the requested architecture while preserving order."""
    return list(
        entry
        for entry in entries
        if entry.entry_type == "stub" and entry.matches_arch(target_arch)
    )


def format_stub(entry: SpecEntry) -> str:
    """Generate the C code for a stub entry."""

    symbol = entry.internal_name
    if not symbol:
        raise ValueError(f"Invalid entry: {entry}")

    export = entry.function_name
    c_identifier = make_c_identifier(symbol)

    # if entry.args is None:
    #     out = "/* Data symbol stub */\n"
    #     if symbol != c_identifier:
    #         out += f"void* {c_identifier} __asm__(\"{symbol}\");\n"
    #     out += f"void* {c_identifier} = NULL;\n"
    #     return out

    params: List[str] = []
    for index, arg in enumerate(entry.argument_types()):
        if arg == "...":
            params.append("...")
            break
        c_type = TYPE_MAP.get(arg, arg)
        params.append(f"{c_type} arg{index}")

    param_list = ", ".join(params) if params else "void"

    line = entry.get_canonical_line()
    return (
        f"// {line}\n"
        f"void {c_identifier}({param_list}) {{\n"
        f'    MSVCRT_UNIMPLEMENTED("{export}");\n'
        "}\n"
    )


def generate_stubs_file(spec_file: str, output_file: str, target_arch: str) -> None:
    # print(f"Reading stubs from {spec_file}...")
    entries = read_spec_file(spec_file)
    selected = select_stub_entries(entries, target_arch)
    # print(f"  Found {len(selected)} stub exports")

    # print(f"\nGenerating {output_file}...")

    with open(output_file, "w", encoding="utf-8") as handle:
        handle.write(STUB_HEADER)

        count = 0
        for entry in selected:
            stub_code = format_stub(entry)
            if not stub_code:
                continue
            handle.write(stub_code)
            handle.write("\n")
            count += 1

    print(f"Generated {count} stubs")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate C stub implementations from Wine .spec file stub entries."
    )
    parser.add_argument("spec_file", help="Path to the input .spec file")
    parser.add_argument("output_file", help="Path to the output .c file")
    parser.add_argument(
        "--arch",
        help="Target architecture to filter exports (e.g. i386, x86_64, arm64).",
        required=True,
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.arch not in ALL_ARCHES:
        raise ValueError(f"Unsupported architecture: {args.arch}")
    generate_stubs_file(args.spec_file, args.output_file, args.arch)


if __name__ == "__main__":
    main()
