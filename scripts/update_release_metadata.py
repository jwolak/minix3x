#!/usr/bin/env python3
import json
import sys
from datetime import datetime
from pathlib import Path


repo_root = Path(__file__).resolve().parent.parent
package_path = repo_root / "package.json"
release_notes_path = repo_root / "RELEASE_NOTES.txt"
if len(sys.argv) != 2:
    raise SystemExit("Usage: update_release_metadata.py COMMIT_SUBJECT")

package = json.loads(package_path.read_text(encoding="utf-8"))
if not isinstance(package, dict):
    raise SystemExit("package.json must contain a JSON object")

version = package.get("version", "")
parts = version.split(".")
if len(parts) != 3 or not all(part.isdigit() for part in parts):
    raise SystemExit(f"Invalid semantic version: {version}")

try:
    release_notes = release_notes_path.read_text(encoding="utf-8").splitlines()
except OSError as error:
    raise SystemExit(f"Cannot read release notes: {error}") from error

subject = sys.argv[1].strip()
if not subject:
    raise SystemExit("Cannot find a commit subject")
subject = subject.replace("|", "-")

major, minor, patch = parts
new_version = f"{major}.{minor}.{int(patch) + 1}"
package["version"] = new_version

timestamp = datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %z")
entry = f"{timestamp} | v{new_version} | {subject}"
insert_at = 0
while insert_at < len(release_notes) and (
    not release_notes[insert_at].strip() or release_notes[insert_at].lstrip().startswith("#")
):
    insert_at += 1
release_notes.insert(insert_at, entry)

package_path.write_text(json.dumps(package, indent=4) + "\n", encoding="utf-8")
release_notes_path.write_text("\n".join(release_notes) + "\n", encoding="utf-8")

print(f"Version bumped to {new_version}; release note added")