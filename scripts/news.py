#!python3

import json
import sys
from dataclasses import dataclass, field
from datetime import UTC, datetime
from pathlib import Path

# region constants
FILE_PATH = Path(__file__).resolve()
ROOT_DIR = FILE_PATH.parents[1]

JSONS_DIR = ROOT_DIR / "jsons"
JSONS_DIR.mkdir(parents=True, exist_ok=True)

NEWS_JSON_FILE = JSONS_DIR / "news.json"
NEWS_CONTENT_JSON_FILE = JSONS_DIR / "news_content.json"
# endregion


# region dts
@dataclass
class Change:
    author: str

    add: list[str] = field(default_factory=list)

    fix: list[str] = field(default_factory=list)

    remove: list[str] = field(default_factory=list)

    tweak: list[str] = field(default_factory=list)

    def empty(self) -> bool:
        return not (self.add or self.fix or self.remove or self.tweak)


# endregion


# region loading
def load_news_ver() -> int:
    if not NEWS_JSON_FILE.exists():
        return 0

    try:
        data = json.loads(NEWS_JSON_FILE.read_text(encoding="utf-8"))

        if not isinstance(data, list) or len(data) != 1 or not isinstance(data[0], int):
            return 0

        return data[0]

    except (OSError, json.JSONDecodeError):
        return 0


def load_news_content() -> list[dict]:
    if not NEWS_CONTENT_JSON_FILE.exists():
        return []

    try:
        data = json.loads(NEWS_CONTENT_JSON_FILE.read_text(encoding="utf-8"))

        if not isinstance(data, list):
            return []

        return data

    except (OSError, json.JSONDecodeError):
        return []


# endregion


# region changelog parsing
def parse_cl(pr_desc: str, pr_author: str) -> Change:
    change = Change(author=pr_author)

    in_cl = False

    for raw_line in pr_desc.splitlines():
        line = raw_line.strip()

        if not in_cl and line.lstrip("\\").lower().startswith(":cl:"):
            in_cl = True
            continue

        if not in_cl:
            continue

        if not line:
            continue

        if line.startswith("---"):
            break

        if not line.startswith("-"):
            continue

        entry = line[1:].strip()

        if ":" not in entry:
            continue

        kind, text = entry.split(":", 1)

        kind = kind.strip().lower()

        text = text.strip()

        if not text:
            continue

        match kind:
            case "add":
                change.add.append(text)

            case "fix":
                change.fix.append(text)

            case "remove" | "rm" | "rem":
                change.remove.append(text)

            case "tweak" | "tw":
                change.tweak.append(text)

    return change


# endregion


# region normalization
def normalize_category(entries: list) -> list[list[str]]:
    authors: dict[str, list[str]] = {}

    for entry in entries:
        if not isinstance(entry, list) or len(entry) < 2:
            continue

        author = entry[0]

        if not isinstance(author, str) or not author.strip():
            continue

        author = author.strip()

        author_entry = authors.setdefault(author, [])

        existing = set(author_entry)

        for text in entry[1:]:
            if not isinstance(text, str):
                continue

            text = text.strip()

            if not text or text in existing:
                continue

            author_entry.append(text)

            existing.add(text)

    return [[author, *texts] for author, texts in authors.items() if texts]


def normalize_content(content: list[dict]) -> bool:
    changed = False

    for day in content:
        if not isinstance(day, dict):
            continue

        for category in ("add", "fix", "rm", "tweak"):
            entries = day.get(category)

            if not isinstance(entries, list):
                day[category] = []
                changed = True
                continue

            normalized = normalize_category(entries)

            if normalized != entries:
                day[category] = normalized

                changed = True

    return changed


# endregion


# region modification
def extend_unique(target: list[list[str]], author: str, texts: list[str]) -> bool:
    if not texts:
        return False

    clean_texts: list[str] = []

    for text in texts:
        if not isinstance(text, str):
            continue

        text = text.strip()

        if not text:
            continue

        clean_texts.append(text)

    if not clean_texts:
        return False

    author_entry = next(
        (
            entry
            for entry in target
            if (isinstance(entry, list) and entry and entry[0] == author)
        ),
        None,
    )

    if author_entry is None:
        author_entry = [author]

        target.append(author_entry)

    existing = set(author_entry[1:])

    changed = False

    for text in clean_texts:
        if text in existing:
            continue

        author_entry.append(text)

        existing.add(text)

        changed = True

    return changed


# endregion


# region sorting
def sort_content(content: list[dict]) -> None:
    for day in content:
        if not isinstance(day, dict):
            continue

        for category in ("add", "fix", "rm", "tweak"):
            entries = day.get(category)

            if not isinstance(entries, list):
                continue

            entries.sort(
                key=lambda item: (
                    item[0].casefold()
                    if (
                        isinstance(item, list)
                        and item
                        and isinstance(
                            item[0],
                            str,
                        )
                    )
                    else ""
                )
            )

    content.sort(
        key=lambda entry: (
            entry.get(
                "date",
                "",
            )
            if isinstance(
                entry,
                dict,
            )
            else ""
        ),
        reverse=True,
    )


# endregion


# region writing
def write_news_content(content: list[dict]) -> None:
    NEWS_CONTENT_JSON_FILE.write_text(
        json.dumps(
            content,
            ensure_ascii=False,
            indent=4,
        )
        + "\n",
        encoding="utf-8",
    )


def write_news_ver(version: int) -> None:
    NEWS_JSON_FILE.write_text(json.dumps([version]) + "\n", encoding="utf-8")


# endregion


def dump_to_json(date: datetime, cl: list[Change]) -> None:
    if not cl:
        return

    content = load_news_content()

    changed = normalize_content(content)

    str_date = date.strftime("%Y-%m-%d")

    day = next(
        (
            entry
            for entry in content
            if (isinstance(entry, dict) and entry.get("date") == str_date)
        ),
        None,
    )

    if day is None:
        day = {
            "date": str_date,
            "add": [],
            "fix": [],
            "rm": [],
            "tweak": [],
        }

        content.append(day)

    for category in ("add", "fix", "rm", "tweak"):
        if not isinstance(
            day.get(category),
            list,
        ):
            day[category] = []
            changed = True

    for change in cl:
        changed |= extend_unique(day["add"], change.author, change.add)
        changed |= extend_unique(day["fix"], change.author, change.fix)
        changed |= extend_unique(day["rm"], change.author, change.remove)
        changed |= extend_unique(day["tweak"], change.author, change.tweak)

    if not changed:
        return

    sort_content(content)

    write_news_content(content)

    manifest_ver = load_news_ver() + 1

    write_news_ver(manifest_ver)


def main(pr_desc: str, pr_author: str) -> None:
    change = parse_cl(pr_desc, pr_author)

    if change.empty():
        print("No changelog entries found.")
        return

    dump_to_json(datetime.now(UTC), [change])

    print(f"News updated. Version: {load_news_ver()}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <author> <pr description file>")

        sys.exit(1)

    author = sys.argv[1]

    desc_file = Path(sys.argv[2])

    main(desc_file.read_text(encoding="utf-8"), author)
