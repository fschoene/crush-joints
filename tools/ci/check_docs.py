#!/usr/bin/env python3
"""Check that relative Markdown links and image targets in the docs exist."""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
FILES = [ROOT / "README.md"]
FILES += sorted((ROOT / "docs").glob("*.md"))
FILES += sorted((ROOT / "tools").rglob("*.md"))

LINK = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")


def main() -> int:
    broken = []

    for path in FILES:
        for target in LINK.findall(path.read_text(encoding="utf-8")):
            target = target.strip().split("#", 1)[0]
            if not target or target.startswith(("http://", "https://", "mailto:")):
                continue

            resolved = (path.parent / target).resolve()
            if not resolved.exists():
                broken.append(f"{path.relative_to(ROOT)}: {target}")

    for item in broken:
        print(f"broken link: {item}", file=sys.stderr)

    if broken:
        return 1

    print(f"checked {len(FILES)} markdown files: no broken relative links")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
