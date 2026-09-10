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


def load_news_ver() -> int:
    if not NEWS_JSON_FILE.exists():
        return 0

    return json.loads(NEWS_JSON_FILE.read_text(encoding="utf-8"))[0]


def load_news_content() -> list[dict]:
    if not NEWS_CONTENT_JSON_FILE.exists():
        return []

    return json.loads(NEWS_CONTENT_JSON_FILE.read_text(encoding="utf-8"))


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

        if not line.startswith("-"):
            continue

        if line.startswith("---"):
            return change

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

            case "remove" | "rm":
                change.remove.append(text)

            case "tweak":
                change.tweak.append(text)

    return change


def extend_unique(
    target: list[list[str]],
    author: str,
    texts: list[str],
) -> bool:
    existing = {(entry[0], entry[1]) for entry in target}

    changed = False

    for text in texts:
        item = (author, text)

        if item in existing:
            continue

        target.append([author, text])
        existing.add(item)
        changed = True

    return changed


def dump_to_json(date: datetime, cl: list[Change]) -> None:
    if not cl:
        return

    content = load_news_content()
    str_date = date.strftime("%Y-%m-%d")

    day = next(
        (entry for entry in content if entry.get("date") == str_date),
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

    changed = False

    for change in cl:
        changed |= extend_unique(day["add"], change.author, change.add)
        changed |= extend_unique(day["fix"], change.author, change.fix)
        changed |= extend_unique(day["rm"], change.author, change.remove)
        changed |= extend_unique(day["tweak"], change.author, change.tweak)

    if not changed:
        return

    for entry in content:
        entry["add"].sort(key=lambda item: item[0].casefold())
        entry["fix"].sort(key=lambda item: item[0].casefold())
        entry["rm"].sort(key=lambda item: item[0].casefold())
        entry["tweak"].sort(key=lambda item: item[0].casefold())

    content.sort(
        key=lambda entry: entry.get("date", ""),
        reverse=True,
    )

    NEWS_CONTENT_JSON_FILE.write_text(
        json.dumps(content, ensure_ascii=False, indent=4) + "\n",
        encoding="utf-8",
    )

    manifest_ver = load_news_ver() + 1

    NEWS_JSON_FILE.write_text(
        json.dumps([manifest_ver]) + "\n",
        encoding="utf-8",
    )


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
