#!/usr/bin/env python3
"""
Merge Wine spec files while avoiding duplicates.

This script combines multiple Wine .spec files, ensuring that:
1. All entries from all files are included
2. Duplicate entries are removed (keeping the first occurrence)
3. Comments and blank lines are preserved
4. The original formatting is maintained
"""

import os
import sys
from typing import Dict, List

from spec_parser import SpecEntry, read_spec_file


def get_file_basename(filepath: str) -> str:
    """Extract the base name from a file path (e.g., 'msvcrt.spec' -> 'msvcrt')."""
    basename = os.path.basename(filepath)
    # Remove .spec extension if present
    if basename.endswith(".spec"):
        basename = basename[:-5]
    return basename


def merge_spec_files(input_files: List[str], output_file: str):
    """Merge multiple spec files, removing duplicates."""

    # Read all input files
    all_file_entries = []
    for input_file in input_files:
        # print(f"Reading {input_file}...")
        entries = read_spec_file(input_file)
        # print(f"  Found {len(entries)} lines")
        all_file_entries.append((input_file, entries))

    export_map: Dict[str, List[SpecEntry]] = {}
    merged_entries = []
    duplicate_count = 0
    replaced_count = 0

    def replace_entry(
        export_key: str,
        old: SpecEntry,
        new: SpecEntry,
        remove: bool,
    ):
        nonlocal replaced_count

        export_list = export_map.setdefault(export_key, [])
        if remove:
            export_list.remove(old)
            merged_entries.remove(old)
        else:
            if old in export_list:
                idx = export_list.index(old)
                export_list[idx] = new
            else:
                export_list.append(new)

            if old in merged_entries:
                idx = merged_entries.index(old)
                merged_entries[idx] = new
            else:
                merged_entries.append(new)

        replaced_count += 1

        # print(f"  Replacing {export_key}")
        # print(f"    Old: {old.get_canonical_line()} ({old.source_file}:{old.line_num})")
        # print(f"    New: {new.get_canonical_line()} ({new.source_file}:{new.line_num})")

    def add_export(entry: SpecEntry) -> bool:
        """Add an export entry, preferring implementations over stubs and warning on conflicts.
        Returns True if added, False if skipped."""
        nonlocal duplicate_count, replaced_count

        export_key = entry.get_export_key()
        if not export_key:
            return False

        arch_coverage = entry.arch_coverage()
        export_list = export_map.setdefault(export_key, [])

        replaces = []
        for idx, existing_entry in enumerate(export_list):
            existing_coverage = existing_entry.arch_coverage()
            if arch_coverage.isdisjoint(existing_coverage):
                # No overlapping architecture coverage, keep both
                continue

            matches_signature = entry.matches_signature(existing_entry)
            if arch_coverage == existing_coverage:
                if matches_signature:
                    # Duplicate entry
                    duplicate_count += 1
                    return False

                if existing_entry.entry_type == "stub" and entry.entry_type != "stub":
                    # Replace stub with implementation
                    replaces.append(existing_entry)
                    continue

                if entry.entry_type == "stub" and existing_entry.entry_type != "stub":
                    # Skip adding stub if implementation exists
                    duplicate_count += 1
                    return False

            if existing_coverage.issubset(arch_coverage):
                if matches_signature:
                    # New entry covers existing one
                    replaces.append(existing_entry)
                    continue

            if arch_coverage.issubset(existing_coverage):
                if matches_signature:
                    # Existing entry covers new one
                    duplicate_count += 1
                    return False

            print(f"  WARNING: Conflicting definitions for {export_key}:")
            print(
                f"    Existing: {existing_entry.get_canonical_line()} ({existing_entry.source_file}:{existing_entry.line_num})"
            )
            print(
                f"    New:      {entry.get_canonical_line()} ({entry.source_file}:{entry.line_num})"
            )
            print("    Keeping existing definition")
            duplicate_count += 1
            return False

        if len(replaces) > 0:
            for i, existing_entry in enumerate(replaces):
                replace_entry(export_key, existing_entry, entry, i != 0)
            return True

        export_list.append(entry)
        merged_entries.append(entry)
        return True

    # Track how many entries were added from each file
    added_per_file = {}

    # Process remaining files
    for input_file, entries in all_file_entries:
        basename = get_file_basename(input_file)

        # Add a separator comment
        merged_entries.append(SpecEntry("", 0, "generated"))
        merged_entries.append(
            SpecEntry(f"# Entries from {basename}.spec", 0, "generated")
        )
        merged_entries.append(SpecEntry("", 0, "generated"))

        # Process entries
        added_count = 0
        for entry in entries:
            if entry.entry_type:
                if add_export(entry):
                    added_count += 1
            else:
                # Keep comments and blank lines
                merged_entries.append(entry)

        added_per_file[basename] = added_count

    # Write the merged file
    # print(f"\nWriting merged output to {output_file}...")
    with open(output_file, "w", encoding="utf-8") as f:
        for entry in merged_entries:
            f.write(entry.line + "\n")

    # print("\nMerge complete!")
    for i, (input_file, entries) in enumerate(all_file_entries):
        basename = get_file_basename(input_file)
        added = added_per_file[basename]
        print(f"Merged {basename}.spec: {added} / {len(entries)}")
    # print(f"  Entries replaced (arch-specific + stubs): {replaced_count}")
    # print(f"  Duplicate entries skipped: {duplicate_count}")
    print(f"Total entries: {len(merged_entries)}")


def main():
    if len(sys.argv) < 3:
        print("Usage: merge_specs.py <input1.spec> [input2.spec ...] <output.spec>")
        print("\nMerges multiple Wine .spec files, removing duplicates.")
        print("The output file is the last argument.")
        print("\nExample:")
        print("  merge_specs.py msvcrt.spec ucrtbase.spec merged.spec")
        print("  merge_specs.py msvcr*.spec ucrtbase.spec output.spec")
        sys.exit(1)

    # Last argument is the output file
    output_file = sys.argv[-1]
    # All previous arguments are input files
    input_files = sys.argv[1:-1]

    if len(input_files) < 1:
        print("Error: At least one input file is required")
        sys.exit(1)

    merge_spec_files(input_files, output_file)


if __name__ == "__main__":
    main()
