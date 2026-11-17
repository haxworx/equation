#!/usr/bin/env python3
"""Format C and EDC sources with Elaro/EFL indentation, or check with --check."""

import argparse
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def equation_braces(line, in_comment):
    """Count code braces without treating comments or literals as code."""
    code = []
    index = 0
    while index < len(line):
        if in_comment:
            end = line.find("*/", index)
            if end < 0:
                break
            in_comment = False
            index = end + 2
        elif line.startswith("/*", index):
            in_comment = True
            index += 2
        elif line.startswith("//", index):
            break
        elif line[index] in "\"'":
            quote = line[index]
            index += 1
            while index < len(line):
                if line[index] == "\\":
                    index += 2
                elif line[index] == quote:
                    index += 1
                    break
                else:
                    index += 1
        else:
            code.append(line[index])
            index += 1
    return "".join(code), in_comment


def equation_format(source, path):
    # GNU's two-space control indentation plus one space per enclosing scope
    # gives EFL's three-space bodies and two-space inset control braces.
    result = subprocess.run(
        ["clang-format", f"--assume-filename={path}"],
        input=source, text=True, capture_output=True, check=True, cwd=ROOT,
    ).stdout
    lines = []
    depth = 0
    in_comment = False
    directive = False
    for line in result.splitlines():
        if directive or line.lstrip().startswith("#"):
            lines.append(line.rstrip())
            directive = line.rstrip().endswith("\\")
            continue
        code, in_comment = equation_braces(line, in_comment)
        leading_closes = len(re.match(r"\s*(}*)", code).group(1))
        indentation = max(0, depth - leading_closes)
        lines.append(" " * indentation + line.rstrip() if line.strip() else "")
        depth += code.count("{") - code.count("}")
    if depth or in_comment:
        raise ValueError(f"Unbalanced source: {path}")
    return "\n".join(lines) + "\n"


def equation_edc_format(source):
    """Keep Elaro's compact EDC statements and one-space nesting."""
    lines = []
    depth = 0
    in_comment = False
    directive = False
    for line in source.splitlines():
        if directive or line.lstrip().startswith("#"):
            lines.append(line.rstrip())
            directive = line.rstrip().endswith("\\")
            continue
        was_comment = in_comment
        code, in_comment = equation_braces(line, in_comment)
        leading = re.match(r"\s*((?:}\s*)*)", code).group(1).count("}")
        indentation = max(0, depth - leading)
        if was_comment:
            indentation += 1
        lines.append(" " * indentation + line.strip() if line.strip() else "")
        depth += code.count("{") - code.count("}")
    if depth or in_comment:
        raise ValueError("Unbalanced EDC source")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="report differences without editing")
    args = parser.parse_args()
    changed = []
    paths = sorted(
        path for directory in ("src", "tests")
        for path in (ROOT / directory).rglob("*") if path.suffix in (".c", ".h")
    )
    paths += sorted((ROOT / "data").rglob("*.edc"))
    for path in paths:
        source = path.read_text()
        formatted = equation_edc_format(source) if path.suffix == ".edc" else equation_format(source, path)
        if formatted != source:
            changed.append(path.relative_to(ROOT))
            if not args.check:
                path.write_text(formatted)
    for path in changed:
        print(path)
    return int(args.check and bool(changed))


if __name__ == "__main__":
    raise SystemExit(main())
