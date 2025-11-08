"""Utilities for parsing Wine .spec entries."""

from __future__ import annotations

import re
from typing import Dict, FrozenSet, List, Optional, Tuple

ARCH_ALIASES = {
    "i486": "i386",
    "i586": "i386",
    "i686": "i386",
    "i786": "i386",
    "amd64": "x86_64",
    "aarch64": "arm64",
}


def normalize_arch_name(name: str) -> str:
    """Normalize an architecture name using ARCH_ALIASES.

    Preserves a leading '!' negation marker if present.
    """
    if not name:
        return name

    negated = name[0] == "!"
    base = name[1:] if negated else name
    lowered = base.lower()
    normalized = ARCH_ALIASES.get(lowered, lowered)
    return f"!{normalized}" if negated else normalized


WIN32_ARCHES: FrozenSet[str] = frozenset({"i386", "arm"})
WIN64_ARCHES: FrozenSet[str] = frozenset({"x86_64", "arm64", "arm64ec"})
SPECIAL_ARCH_GROUPS = {
    "win32": WIN32_ARCHES,
    "win64": WIN64_ARCHES,
}

ALL_ARCHES: FrozenSet[str] = frozenset(WIN32_ARCHES | WIN64_ARCHES)

TYPE_MAP: Dict[str, str] = {
    "word": "unsigned short",
    "s_word": "short",
    "segptr": "void *",
    "segstr": "const char *",
    "long": "long",
    "ptr": "void *",
    "str": "const char *",
    "wstr": "const wchar_t *",
    "int64": "__int64",
    "int128": "__int128",
    "float": "float",
    "double": "double",
}


def expand_arch(name: str) -> FrozenSet[str]:
    """Expand an architecture or architecture group, preserving negation."""
    normalized = normalize_arch_name(name)
    if not normalized:
        return frozenset()

    negated = normalized.startswith("!")
    base = normalized[1:] if negated else normalized

    if base in SPECIAL_ARCH_GROUPS:
        entries = SPECIAL_ARCH_GROUPS[base]
    else:
        entries = (base,)

    if negated:
        return frozenset(f"!{entry}" for entry in entries)
    return frozenset(entries)


def get_pointer_size(arch: str) -> int:
    if arch in WIN32_ARCHES:
        return 4
    elif arch in WIN64_ARCHES:
        return 8
    else:
        raise ValueError(f"Unknown architecture: {arch}")


class SpecEntry:
    """Represents a single entry in a Wine .spec file."""

    def __init__(self, line: str, line_num: int, source_file: str):
        self.line = line.rstrip()
        self.line_num = line_num
        self.source_file = source_file

        # Parsed components of export entries
        self.entry_type: Optional[str] = (
            None  # cdecl, stub, varargs, stdcall, thiscall, extern
        )
        self.modifiers: List[str] = []  # -ret64, -arch=X, -norelay, -private, etc.
        self.function_name: Optional[str] = (
            None  # Exported function name (without args)
        )
        self.args: Optional[str] = None  # Arguments including parentheses
        self.internal_name: Optional[str] = (
            None  # Optional internal implementation name
        )
        self.comment: Optional[str] = None  # Comment after #

        if self.line.strip().startswith("@"):
            self._parse_export_entry()

    def _parse_export_entry(self) -> None:
        """Parse a Wine spec export entry into its components."""
        line = self.line.strip()

        if not line.startswith("@"):
            return

        content = line[1:].lstrip()
        content, comment = self._split_comment(content)
        self.comment = comment

        tokens = self._tokenize(content)
        if not tokens:
            return

        self.entry_type = tokens[0]

        idx = 1
        while idx < len(tokens) and tokens[idx].startswith("-"):
            modifier = tokens[idx]
            self.modifiers.append(modifier)
            idx += 1

        if idx < len(tokens):
            func_token = tokens[idx]
            idx += 1

            paren_pos = func_token.find("(")
            if paren_pos != -1:
                self.function_name = func_token[:paren_pos]
                self.args = func_token[paren_pos:]
            else:
                self.function_name = func_token

        if (
            self.function_name
            and self.args is None
            and idx < len(tokens)
            and tokens[idx].startswith("(")
        ):
            self.args = tokens[idx]
            idx += 1

        if idx < len(tokens):
            self.internal_name = tokens[idx]

        if not self.internal_name and self.function_name:
            self.internal_name = self.function_name

    @staticmethod
    def _split_comment(text: str) -> Tuple[str, Optional[str]]:
        """Split text into content and a trailing comment introduced by #."""
        paren_depth = 0
        for index, char in enumerate(text):
            if char == "(":
                paren_depth += 1
            elif char == ")":
                if paren_depth > 0:
                    paren_depth -= 1
            elif char == "#" and paren_depth == 0:
                return text[:index].rstrip(), text[index + 1 :].strip()
        return text.strip(), None

    @staticmethod
    def _tokenize(text: str) -> List[str]:
        tokens: List[str] = []
        current: List[str] = []
        paren_depth = 0

        for char in text:
            if char == "(":
                paren_depth += 1
                current.append(char)
            elif char == ")":
                current.append(char)
                if paren_depth > 0:
                    paren_depth -= 1
            elif char.isspace() and paren_depth == 0:
                if current:
                    tokens.append("".join(current))
                    current = []
            else:
                current.append(char)

        if current:
            tokens.append("".join(current))

        return tokens

    def get_export_key(self) -> Optional[str]:
        """Get the export name for deduplication."""
        return self.function_name

    def matches_signature(self, other: "SpecEntry") -> bool:
        """Check if this entry has the same signature as another."""
        return (
            self.entry_type == other.entry_type
            and self._non_arch_modifiers() == other._non_arch_modifiers()
            and self.function_name == other.function_name
            and (self.args or "") == (other.args or "")
            and (self.internal_name or "") == (other.internal_name or "")
        )

    def _non_arch_modifiers(self) -> Tuple[str, ...]:
        mods = [m for m in self.modifiers if not m.startswith("-arch=")]
        mods.sort()
        return tuple(mods)

    def get_canonical_line(self) -> str:
        """Reconstruct the canonical line without comments."""
        if not self.entry_type:
            return self.line

        parts: List[str] = ["@", self.entry_type]
        parts.extend(self.modifiers)

        func_part = self.function_name or ""
        if self.args:
            func_part += self.args
        if func_part:
            parts.append(func_part)

        if self.internal_name and (
            self.function_name is None or self.internal_name != self.function_name
        ):
            parts.append(self.internal_name)

        return " ".join(parts)

    def has_arch_modifier(self) -> bool:
        """Return True if any -arch modifier is present."""
        for modifier in self.modifiers:
            if modifier.startswith("-arch="):
                return True
        return False

    def arch_targets(self) -> Tuple[str, ...]:
        """Return a sorted tuple of normalized architectures specified by -arch."""
        targets = set()
        for modifier in self.modifiers:
            if modifier.startswith("-arch="):
                for part in modifier[len("-arch=") :].split(","):
                    name = part.strip()
                    if not name:
                        continue
                    targets.update(expand_arch(name))
        return tuple(sorted(targets))

    def arch_coverage(self) -> FrozenSet[str]:
        """Return the concrete architecture set covered by this entry."""
        include = set()
        exclude = set()
        for target in self.arch_targets():
            if target.startswith("!"):
                exclude.update(expand_arch(target))
            else:
                include.update(expand_arch(target))

        include_positive = {arch for arch in include if not arch.startswith("!")}
        exclude_positive = {arch for arch in exclude if arch.startswith("!")}

        if not include_positive:
            # All modifiers were exclusions; include everything else.
            include_positive = set(ALL_ARCHES)

        coverage = include_positive - {arch[1:] for arch in exclude_positive}
        return frozenset(coverage)

    def argument_types(self) -> List[str]:
        """Return the list of argument specifiers, without parentheses."""
        if not self.args or len(self.args) < 2:
            return []
        inner = self.args[1:-1].strip()
        if not inner:
            return []
        return inner.split()

    def arguments_size(self, arch: str) -> int:
        """Return the number of arguments."""
        pointer_size = get_pointer_size(arch)
        size = 0
        for arg in self.argument_types():
            if arg == "int64" or arg == "double":
                size += 8
            elif arg == "int128":
                if arch == "x86_64":
                    # Passed as pointer
                    size += pointer_size
                else:
                    size += 16
            else:
                size += pointer_size
        return size

    def matches_arch(self, target_arch: str) -> bool:
        """Return True if the entry applies to the requested architecture."""
        if not self.has_arch_modifier():
            return True

        return target_arch in self.arch_coverage()

    def __repr__(self) -> str:
        return f"SpecEntry({self.function_name or self.line!r}, {self.source_file}:{self.line_num})"


def read_spec_file(filename: str) -> List[SpecEntry]:
    """Read a spec file and return a list of SpecEntry objects."""
    entries: List[SpecEntry] = []
    with open(filename, "r", encoding="utf-8") as handle:
        for line_num, line in enumerate(handle, 1):
            entries.append(SpecEntry(line, line_num, filename))
    return entries


def make_c_identifier(symbol: str) -> str:
    """Return a valid C identifier for the given symbol."""

    base = re.sub(r"[^0-9A-Za-z_]", "_", symbol)
    if not base:
        base = "_stub"
    if base[0].isdigit():
        base = f"_{base}"

    return base


__all__ = [
    "ARCH_ALIASES",
    "WIN32_ARCHES",
    "WIN64_ARCHES",
    "SPECIAL_ARCH_GROUPS",
    "ALL_ARCHES",
    "normalize_arch_name",
    "expand_arch",
    "SpecEntry",
    "read_spec_file",
    "make_c_identifier",
]
