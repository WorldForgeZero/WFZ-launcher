#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
from pathlib import Path


def escape_cpp_string(value: str) -> str:
    return (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "\\r")
        .replace("\t", "\\t")
    )


def format_bytes(data: bytes | bytearray) -> list[str]:
    lines: list[str] = []

    for offset in range(0, len(data), 16):
        chunk = data[offset : offset + 16]

        values = ", ".join(f"0x{value:02x}" for value in chunk)

        lines.append(f"    {values},")

    return lines


def normalize_resource_path(
    path: Path,
) -> str:
    if path.is_absolute():
        raise RuntimeError(f"Resource path must be relative: {path}")

    normalized = path.as_posix()

    while normalized.startswith("./"):
        normalized = normalized[2:]

    if not normalized:
        raise RuntimeError(f"Invalid resource path: {path}")

    return normalized


def minify_rml(text: str) -> str:
    text = re.sub(r"<!--.*?-->", "", text, flags=re.DOTALL)

    text = re.sub(r">\s+<", "><", text)

    return text.strip()


def minify_rcss(text: str) -> str:
    output: list[str] = []

    index = 0
    quote: str | None = None
    escaped = False
    pending_space = False

    no_space_around = set("{}:;,>+~()")

    while index < len(text):
        char = text[index]

        if quote is not None:
            output.append(char)

            if escaped:
                escaped = False

            elif char == "\\":
                escaped = True

            elif char == quote:
                quote = None

            index += 1
            continue

        if char == "/" and index + 1 < len(text) and text[index + 1] == "*":
            end = text.find("*/", index + 2)

            if end == -1:
                raise RuntimeError("Unterminated RCSS comment")

            pending_space = True
            index = end + 2
            continue

        if char in ("'", '"'):
            if pending_space and output and output[-1] not in no_space_around:
                output.append(" ")

            pending_space = False
            quote = char
            output.append(char)

            index += 1
            continue

        if char.isspace():
            pending_space = True
            index += 1
            continue

        if pending_space:
            if (
                output
                and output[-1] not in no_space_around
                and char not in no_space_around
            ):
                output.append(" ")

            pending_space = False

        if char in no_space_around and output and output[-1] == " ":
            output.pop()

        output.append(char)
        index += 1

    return "".join(output).strip()


def load_resource(
    path: Path,
) -> bytes:
    suffix = path.suffix.lower()

    if suffix == ".rml":
        text = path.read_text(encoding="utf-8-sig")

        return minify_rml(text).encode("utf-8")

    if suffix == ".rcss":
        text = path.read_text(encoding="utf-8-sig")

        return minify_rcss(text).encode("utf-8")

    return path.read_bytes()


def generate(resources: list[Path]) -> tuple[str, int, int]:
    data = bytearray()

    entries: list[tuple[str, int, int]] = []

    seen_paths: set[str] = set()

    input_size = 0

    sorted_resources = sorted(resources, key=lambda path: path.as_posix())

    for path in sorted_resources:
        if not path.is_file():
            raise RuntimeError(f"Resource file does not exist: {path}")

        resource_path = normalize_resource_path(path)

        if resource_path in seen_paths:
            raise RuntimeError(f"Duplicate resource path: {resource_path}")

        seen_paths.add(resource_path)

        input_size += path.stat().st_size

        content = load_resource(path)

        offset = len(data)

        data.extend(content)

        entries.append((resource_path, offset, len(content)))

    lines: list[str] = [
        "// Generated file. Do not edit.",
        "",
        "constexpr std::uint8_t EMBEDDED_DATA[] =",
        "{",
    ]

    if data:
        lines.extend(format_bytes(data))

    else:
        lines.append("    0x00,")

    lines.extend(["};", "", "constexpr Entry EMBEDDED_ENTRIES[] =", "{"])

    for path, offset, size in entries:
        escaped_path = escape_cpp_string(path)

        lines.append(f'    {{"{escaped_path}", {offset}, {size}}},')

    lines.extend(
        [
            "};",
            "",
            (f"constexpr std::size_t EMBEDDED_ENTRY_COUNT = {len(entries)};"),
            "",
        ]
    )

    return ("\n".join(lines), input_size, len(data))


def main() -> None:
    parser = argparse.ArgumentParser()

    parser.add_argument("--output", required=True, type=Path)

    parser.add_argument("resources", nargs="+", type=Path)

    args = parser.parse_args()

    output: Path = args.output

    resources: list[Path] = args.resources

    (generated, input_size, embedded_size) = generate(resources)

    output.parent.mkdir(parents=True, exist_ok=True)

    with output.open("w", encoding="utf-8", newline="\n") as file:
        file.write(generated)

    saved = input_size - embedded_size

    print(
        "Embedded "
        f"{len(resources)} resources: "
        f"{input_size} -> "
        f"{embedded_size} bytes "
        f"({saved} bytes removed)"
    )


if __name__ == "__main__":
    main()
