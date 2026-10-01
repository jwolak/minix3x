#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(git -C "$script_dir" rev-parse --show-toplevel)"

if ! command -v clang-format >/dev/null 2>&1; then
	echo "clang-format is not installed or not in PATH." >&2
	exit 127
fi

git -C "$repo_root" ls-files --cached --others --exclude-standard -z -- '*.c' '*.h' |
	xargs -0 -r -n 100 clang-format -style=file -i --